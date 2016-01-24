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

/// Default constructor.
ShaderGenerator::ShaderGenerator()
{
}

/// Destructor.
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

/// Generate all of the necessary shaders for the current mesh. After a shader has been generated,
/// it is automatically setup and bound to the proper mesh piece.
bool ShaderGenerator::generateShaders(gfx::Mesh &mesh, 
                                      gfx::Shader &vertex_shader, 
                                      gfx::Buffer &light_buffer,
                                      gfx::Buffer &gi_buffer, 
                                      const std::string &ray_shader_path,
                                      const std::string &light_shader_path,
                                      const int max_light_count,
                                      std::vector<Light> &lights)
{
    std::string ray_shader_source;
    std::string light_shader_source;
    if (!util::ReadTextFile(ray_shader_path, ray_shader_source))
    {
        std::cout << "Unable to read ray shader at path: " << ray_shader_path;
        return false;
    }

    if (!util::ReadTextFile(light_shader_path, light_shader_source))
    {
        std::cout << "Unable to read light shader at path: " << light_shader_path;
        return false;
    }

    int light_count = 0;

    // Iterate over each mesh piece and create a shader for it. Afterwards bind the VBOs to the proper
    // locations in the newly compiled programs.
    gfx::Mesh::MeshList &meshes = mesh.getMeshList();
    for (gfx::Mesh::MeshList::iterator iter = meshes.begin(); iter != meshes.end(); ++iter)
    {
        gfx::Mesh::MeshPiece *piece = &(iter->second);
        rlBindPrimitive(RL_PRIMITIVE, piece->primitive);
        
        if (piece->material.name.find("Light") == std::string::npos)
        {
            // Create the ray shader based on the materials from this mesh piece.
            gfx::Shader *ray_shader = findOrCreateRayShader(piece->material, ray_shader_source, max_light_count);
            if (ray_shader == nullptr)
            {
                return false;
            }

            piece->program.attach(vertex_shader);
            piece->program.attach(*ray_shader);
            piece->program.link(piece->material.name);
            rlUseProgram(piece->program.getProgram());
            
            // Attach the uniform buffer for the light position to this primitive.
            RLuint light_block_index = rlGetUniformBlockIndex(piece->program.getProgram(), "Light");
            rlUniformBlockBuffer(light_block_index, light_buffer.GetBuffer());
        }
        else
        {
            // Setup the lighting primitive and program to use along with it.
            rlPrimitiveParameter1i(RL_PRIMITIVE, RL_PRIMITIVE_IS_OCCLUDER, RL_FALSE);
            
            // Set the corresponding light primitive to this new primitive.
            lights[light_count++].primitive = piece->primitive;

            // Create the light shader for this light source.
            gfx::Shader *light_shader = findOrCreateRayShader(piece->material, light_shader_source, max_light_count);
            if (light_shader == nullptr)
            {
                return false;
            }
            
            piece->program.attach(*light_shader);
            piece->program.attach(vertex_shader);
            piece->program.link("light");
            rlUseProgram(piece->program.getProgram());
        }
        
        // Set the shader's uniforms.
        if (piece->material.componentFlags.test(gfx::Material::DIFFUSE))
        {
            piece->program.set3fv("kd", piece->material.diffuse.v);
            
            if (piece->material.diffuseTexture.isValid())
            {
                piece->program.setTexture("diffuseTexture", piece->material.diffuseTexture.getTexture());
            }
            if (piece->material.normalTexture.isValid())
            {
            	piece->program.setTexture("normalmap", piece->material.normalTexture.getTexture());
            }
            /*if (piece->material.component_flags.test(gfx::Material::SUBSURFACE))
            {
                piece->program.set1i("subsurface", 1);
                piece->program.setPrimitive("subsurface_primitive", subsurface_prim);
            }*/
            
            // Attach the uniform buffer for the random data to this primitive. Also setup the bounce probability
            // for russian roulette for this particual material.
            RLuint gi_block_index = rlGetUniformBlockIndex(piece->program.getProgram(), "GI");
            if (gi_block_index != ~0) // All 0xF's returned if this block is invalid (this could be a light source).
            {
                rlUniformBlockBuffer(gi_block_index, gi_buffer.GetBuffer());
                
                float bounce_probablility = (piece->material.diffuse[0] + piece->material.diffuse[1] + piece->material.diffuse[2]) / 3.0f;
                piece->program.set1f("bounceProbablility", bounce_probablility);
            }
        }
        if (piece->material.componentFlags.test(gfx::Material::SPECULAR))
        {
            math::Vec4f uniform(piece->material.specular);
            uniform[3] = piece->material.roughness;
            piece->program.set4fv("ks", uniform.v);
        }
        if (piece->material.componentFlags.test(gfx::Material::TRANSMISSIVE))
        {
            math::Vec4f uniform(piece->material.transmissive);
            uniform[3] = piece->material.indexOfRefraction;
            piece->program.set4fv("kt", uniform.v);
        }
        
        // Attach the proper vbos to the attributes for this shader.
        gfx::Mesh::RenderData render_data;
        render_data.position_attribute  = rlGetAttribLocation(piece->program.getProgram(), "positionAttribute");
        render_data.normal_attribute    = rlGetAttribLocation(piece->program.getProgram(), "normalAttribute");
        render_data.tex_coord_attribute = rlGetAttribLocation(piece->program.getProgram(), "texCoordAttribute");
        render_data.tangent_attribute   = rlGetAttribLocation(piece->program.getProgram(), "tangentAttribute");
        
        piece->buffers[gfx::Mesh::VERTICES].Bind();
        rlVertexAttribBuffer(render_data.position_attribute, 3, RL_FLOAT, RL_FALSE, sizeof(math::Vec3f), 0);
        piece->buffers[gfx::Mesh::VERTICES].Unbind();
        
        piece->buffers[gfx::Mesh::NORMALS].Bind();
        rlVertexAttribBuffer(render_data.normal_attribute, 3, RL_FLOAT, RL_FALSE, sizeof(math::Vec3f), 0);
        piece->buffers[gfx::Mesh::NORMALS].Unbind();
        
        piece->buffers[gfx::Mesh::TEX_COORDS].Bind();
        rlVertexAttribBuffer(render_data.tex_coord_attribute, 2, RL_FLOAT, RL_FALSE, sizeof(math::Vec2f), 0);
        piece->buffers[gfx::Mesh::TEX_COORDS].Unbind();
        
        piece->buffers[gfx::Mesh::TANGENTS].Bind();
        rlVertexAttribBuffer(render_data.tangent_attribute, 3, RL_FLOAT, RL_FALSE, sizeof(math::Vec3f), 0);
        piece->buffers[gfx::Mesh::TANGENTS].Unbind();
        
        // Submit this mesh to OpenRL for heirarchy building and later on rendering.
        rlDrawArrays(RL_TRIANGLES, 0, piece->num_elements);
        
        rlBindPrimitive(RL_PRIMITIVE, RL_NULL_PRIMITIVE);
        
        CheckRLErrors();
    }
    
    return true;
}

/// Generate a ray shader based on a material OR find the existing shader in the shader cache. If a ray shader was not found and
/// count not be created, null is returned.
gfx::Shader *ShaderGenerator::findOrCreateRayShader(const gfx::Material &material, const std::string &shader_source, const int max_light_count)
{
    // Look for this shader in the cache first.
    ShaderFlags shader_flags = material.componentFlags.to_ulong();
    ShaderCache::iterator iter = m_shaderCache.find(shader_flags);

    if (iter != m_shaderCache.end())
    {
        // This shader already exists, use it.
        return iter->second;
    }

    // The shader doesn't already exist, so create one.
    std::stringstream stream;
    stream << "#define MAX_LIGHTS " << max_light_count << std::endl;
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

    final_source += shader_source;

    // create the shader from the assembled shader source string.
    gfx::Shader *shader = new gfx::Shader;
    if (!shader->createFromString(final_source, gfx::Shader::RAY, material.name))
    {
        delete shader;
        shader = nullptr;
        return nullptr;
    }

    m_shaderCache[shader_flags] = shader;
    return shader;
}
