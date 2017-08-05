//
//  ShaderGenerator.cpp
//  Heatray
//
//  Created by Cody White on 5/28/14.
//
//

#include "ShaderGenerator.h"
#include "../utility/util.h"
#include <assert.h>
#include <sstream>

ShaderGenerator::ShaderGenerator()
{
}

ShaderGenerator::~ShaderGenerator()
{
    ShaderCache::iterator iter = m_shaderCache.begin();
    for (; iter != m_shaderCache.end(); ++iter)
    {
        delete iter->second;
        iter->second = nullptr;
    }

    m_shaderCache.clear();
}

bool ShaderGenerator::GenerateShaders(const GenerationInfo &info)
{
    std::string rayShaderSource;
    std::string lightShaderSource;
    if (!util::ReadTextFile(info.rayShaderPath, rayShaderSource))
    {
        std::cout << "Unable to read ray shader at path: " << info.rayShaderPath;
        return false;
    }

    if (!util::ReadTextFile(info.lightShaderPath, lightShaderSource))
    {
        std::cout << "Unable to read light shader at path: " << info.lightShaderPath;
        return false;
    }

    int lightCount = 0;

    // Iterate over each mesh piece and create a shader for it. Afterwards bind the VBOs to the proper
    // locations in the newly compiled programs.
    gfx::Mesh::MeshList &meshes = info.mesh->GetMeshList();
    for (gfx::Mesh::MeshList::iterator piece = meshes.begin(); piece != meshes.end(); ++piece)
    {
        rlBindPrimitive(RL_PRIMITIVE, piece->primitive);
        
        if (piece->material.name.find("Light") == std::string::npos)
        {
            // Create the ray shader based on the materials from this mesh piece.
            gfx::Shader *ray_shader = FindOrCreateRayShader(piece->material, rayShaderSource, info.maxLightCount);
            if (ray_shader == nullptr)
            {
                return false;
            }

            piece->program.Attach(*(info.vertexShader));
            piece->program.Attach(*ray_shader);
            piece->program.Link(piece->material.name);
            rlUseProgram(piece->program.GetProgram());
            
            // Attach the uniform buffer for the light position to this primitive.
            RLuint light_block_index = rlGetUniformBlockIndex(piece->program.GetProgram(), "Light");
            rlUniformBlockBuffer(light_block_index, info.lightBuffer->GetBuffer());
        }
        else
        {
            if ((lightCount + 1) > info.maxLightCount)
            {
                std::cout << "Too many lights specified in the mesh. Maximum count allowed is " << info.maxLightCount << std::endl;
                return false;
            }
        
            // Setup the lighting primitive and program to use along with it.
            rlPrimitiveParameter1i(RL_PRIMITIVE, RL_PRIMITIVE_IS_OCCLUDER, RL_FALSE);
            
            // Set the corresponding light primitive to this new primitive.
            info.lights->at(lightCount++).primitive = piece->primitive;

            // Create the light shader for this light source.
            gfx::Shader *lightShader = FindOrCreateRayShader(piece->material, lightShaderSource, info.maxLightCount);
            if (lightShader == nullptr)
            {
                return false;
            }
            
            piece->program.Attach(*(info.vertexShader));
            piece->program.Attach(*lightShader);
            piece->program.Link("light");
            rlUseProgram(piece->program.GetProgram());
        }
        
        // Set the shader's uniforms.
        if (piece->material.componentFlags.test(gfx::Material::DIFFUSE))
        {
            RLint location = piece->program.GetUniformLocation("kd");
            piece->program.Set3fv(location, piece->material.diffuse.v);
            
            if (piece->material.componentFlags.test(gfx::Material::DIFFUSE_TEXTURE))
            {
                location = piece->program.GetUniformLocation("diffuseTexture");
                piece->program.SetTexture(location, piece->material.diffuseTexture.GetTexture());
            }
            if (piece->material.componentFlags.test(gfx::Material::NORMALMAP) &&
                !piece->material.componentFlags.test(gfx::Material::LIGHT)) // Lights do not yet support normal maps.
            {
                location = piece->program.GetUniformLocation("normalmap");
            	piece->program.SetTexture(location, piece->material.normalTexture.GetTexture());
            }
            /*if (piece->material.component_flags.test(gfx::Material::SUBSURFACE))
            {
                piece->program.set1i("subsurface", 1);
                piece->program.setPrimitive("subsurface_primitive", subsurface_prim);
            }*/
            
            // Attach the uniform buffer for the random data to this primitive. Also setup the bounce probability
            // for russian roulette for this particual material.
            RLuint giBlockIndex = rlGetUniformBlockIndex(piece->program.GetProgram(), "GI");
            if (giBlockIndex != ~0) // All 0xF's returned if this block is invalid (this could be a light source).
            {
                rlUniformBlockBuffer(giBlockIndex, info.giBuffer->GetBuffer());
                
                float bounce_probablility = (piece->material.diffuse[0] + piece->material.diffuse[1] + piece->material.diffuse[2]) / 3.0f;
                location = piece->program.GetUniformLocation("bounceProbablility");
                piece->program.Set1f(location, bounce_probablility);
            }
        }
        /*
        if (piece->material.componentFlags.test(gfx::Material::SPECULAR))
        {
            math::Vec4f uniform(piece->material.specular);
            uniform[3] = piece->material.roughness;
            piece->program.Set4fv(piece->program.GetUniformLocation("ks"), uniform.v);
        }
        if (piece->material.componentFlags.test(gfx::Material::TRANSMISSIVE))
        {
            math::Vec4f uniform(piece->material.transmissive);
            uniform[3] = piece->material.indexOfRefraction;
            piece->program.Set4fv(piece->program.GetUniformLocation("kt"), uniform.v);
        }*/
        
        // Attach the proper vbos to the attributes for this shader.
        gfx::Mesh::RenderData renderData;
        renderData.positionAttribute  = rlGetAttribLocation(piece->program.GetProgram(), "positionAttribute");
        renderData.normalAttribute    = rlGetAttribLocation(piece->program.GetProgram(), "normalAttribute");
        renderData.texCoordAttribute  = rlGetAttribLocation(piece->program.GetProgram(), "texCoordAttribute");
        renderData.tangentAttribute   = rlGetAttribLocation(piece->program.GetProgram(), "tangentAttribute");
        renderData.bitangentAttribute = rlGetAttribLocation(piece->program.GetProgram(), "bitangentAttribute");
        
        piece->buffers[gfx::Mesh::kVertices].Bind();
        rlVertexAttribBuffer(renderData.positionAttribute, 3, RL_FLOAT, RL_FALSE, sizeof(math::Vec3f), 0);
        piece->buffers[gfx::Mesh::kVertices].Unbind();
        
        piece->buffers[gfx::Mesh::kNormals].Bind();
        rlVertexAttribBuffer(renderData.normalAttribute, 3, RL_FLOAT, RL_FALSE, sizeof(math::Vec3f), 0);
        piece->buffers[gfx::Mesh::kNormals].Unbind();
        
        if (piece->buffers[gfx::Mesh::kTexCoords].IsValid())
        {
            // If tex coords are present then so will tangents and bitangents.

            piece->buffers[gfx::Mesh::kTexCoords].Bind();
            rlVertexAttribBuffer(renderData.texCoordAttribute, 2, RL_FLOAT, RL_FALSE, sizeof(math::Vec2f), 0);
            piece->buffers[gfx::Mesh::kTexCoords].Unbind();

            piece->buffers[gfx::Mesh::kTangents].Bind();
            rlVertexAttribBuffer(renderData.tangentAttribute, 3, RL_FLOAT, RL_FALSE, sizeof(math::Vec3f), 0);
            piece->buffers[gfx::Mesh::kTangents].Unbind();

            piece->buffers[gfx::Mesh::kBitangents].Bind();
            rlVertexAttribBuffer(renderData.bitangentAttribute, 3, RL_FLOAT, RL_FALSE, sizeof(math::Vec3f), 0);
            piece->buffers[gfx::Mesh::kBitangents].Unbind();
        }

        piece->buffers[gfx::Mesh::kIndices].SetTarget(RL_ELEMENT_ARRAY_BUFFER);
        piece->buffers[gfx::Mesh::kIndices].Bind();
        
        // Submit this mesh to OpenRL for heirarchy building and later on rendering.
        rlDrawElements(RL_TRIANGLES, piece->numIndices, RL_UNSIGNED_INT, 0);

        piece->buffers[gfx::Mesh::kIndices].Unbind();
        
        rlBindPrimitive(RL_PRIMITIVE, RL_NULL_PRIMITIVE);
        
        CheckRLErrors();
    }
    
    return true;
}

gfx::Shader *ShaderGenerator::FindOrCreateRayShader(const gfx::Material &material, const std::string &shaderSource, const int maxLightCount)
{
    // Look for this shader in the cache first.
    ShaderFlags shaderFlags = material.componentFlags.to_ulong();
    ShaderCache::iterator iter = m_shaderCache.find(shaderFlags);

    if (iter != m_shaderCache.end())
    {
        // This shader already exists, use it.
        return iter->second;
    }

    // The shader doesn't already exist, so create one.
    std::stringstream stream;
    stream << "#define MAX_LIGHTS " << maxLightCount << std::endl;
    std::string final_source = stream.str();

    // Add the proper defines to the top of the shader for this material.
    for (int ii = 0; ii < gfx::Material::NUM_COMPONENT_FLAGS; ++ii)
    {
        if (material.componentFlags.test(ii))
        {
            // Essentially write "#define MATERIAL_TYPE".
            final_source += "#define ";
            final_source += gfx::Material::materialNames[ii];
            final_source += "\n";
        }
    }

    final_source += shaderSource;

    // create the shader from the assembled shader source string.
    gfx::Shader *shader = new gfx::Shader;
    if (!shader->CreateFromString(final_source, gfx::Shader::ShaderType::kRay, material.name))
    {
        delete shader;
        shader = nullptr;
        return nullptr;
    }

    m_shaderCache[shaderFlags] = shader;
    return shader;
}
