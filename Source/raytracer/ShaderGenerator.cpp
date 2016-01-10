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

/// Default constructor.
ShaderGenerator::ShaderGenerator()
{
}

/// Destructor.
ShaderGenerator::~ShaderGenerator()
{
}

/// Generate all of the necessary shaders for the current mesh. After a shader has been generated,
/// it is automatically setup and bound to the proper mesh piece.
bool ShaderGenerator::generateShaders(gfx::Mesh &mesh, gfx::Shader &vertex_shader, gfx::Buffer &light_buffer, gfx::Buffer &gi_buffer, std::vector<Light> &lights)
{
    int light_count = 0;
    
    // Attach a shader to the light primitive. This shader either shaders the light directly or
    // accumulates the incoming color based on if this intersection is a shadow occlusion query
    // or not.
    std::string light_shader_with_texture_source = 
        "rayattribute vec3 color; rayattribute bool is_diffuse_bounce; uniform vec3 kd; uniform primitive defaultPrim; uniform sampler2D diffuse_texture; varying vec2 tex_coords; \n"
        "void main()\n"
        "{\n"
        "    accumulate(rl_InRay.color * kd * texture2D(diffuse_texture, tex_coords).zyx);\n"
        "}\n";
    std::string light_shader_without_texture_source = 
        "rayattribute vec3 color; rayattribute bool is_diffuse_bounce; uniform vec3 kd; uniform primitive defaultPrim; \n"
        "void main()\n"
        "{\n"
        "    accumulate(rl_InRay.color * kd);\n"
        "}\n";

    RLprimitive subsurface_prim;
    {
//        rlGenPrimitives(1, &subsurface_prim);
//        gfx::Program subsurface_program;
//        subsurface_program.addShader("Resources/shaders/subsurface.psh", gfx::Shader::PREFIX);
//        subsurface_program.link();
//        rlBindPrimitive(RL_PRIMITIVE, subsurface_prim);
//        rlUseProgram(subsurface_program.getProgram());
    }

    // Iterate over each mesh piece and create an RL primitive object to encapsulate this part of the mesh.
    // For each primitive created, attach the proper VBOs to it.
    gfx::Mesh::MeshList &meshes = mesh.getMeshList();
    for (gfx::Mesh::MeshList::iterator iter = meshes.begin(); iter != meshes.end(); ++iter)
    {
        gfx::Mesh::MeshPiece *piece = &(iter->second);
        
        if (piece->material.name.find("Light") == std::string::npos)
        {
            rlGenPrimitives(1, &(piece->primitive));
            rlBindPrimitive(RL_PRIMITIVE, piece->primitive);
            rlPrimitiveParameterString(RL_PRIMITIVE, RL_PRIMITIVE_NAME, piece->material.name.c_str());
            
            // Generate any textures that this mesh needs.
            piece->material.diffuse_texture.createFromLoadedData(true);
            piece->material.normal_texture.createFromLoadedData(true);
            
            // Create the ray shader based on the materials from this mesh piece.
            gfx::Shader ray_shader;
            if (!generateRayShader(piece->material, ray_shader))
            {
                return false;
            }

            piece->program.attach(vertex_shader);
            piece->program.attach(ray_shader);
            piece->program.link(piece->material.name);
            rlUseProgram(piece->program.getProgram());
            
            // Attach the uniform buffer for the light position to this primitive.
            RLuint light_block_index = rlGetUniformBlockIndex(piece->program.getProgram(), "Light");
            rlUniformBlockBuffer(light_block_index, light_buffer.getBuffer());
        }
        else
        {
            // Setup the lighting primitive and program to use along with it.
            rlGenPrimitives(1, &(piece->primitive));
            rlBindPrimitive(RL_PRIMITIVE, piece->primitive);
            rlPrimitiveParameterString(RL_PRIMITIVE, RL_PRIMITIVE_NAME, piece->material.name.c_str());
            rlPrimitiveParameter1i(RL_PRIMITIVE, RL_PRIMITIVE_IS_OCCLUDER, RL_FALSE);

            // Generate any textures that this mesh needs.
            piece->material.diffuse_texture.createFromLoadedData(true);
            
            // Set the corresponding light primitive to this new primitive.
            lights[light_count++].primitive = piece->primitive;
            
            gfx::Shader light_shader;
            light_shader.createFromString(piece->material.diffuse_texture.isValid() ? light_shader_with_texture_source : light_shader_without_texture_source, gfx::Shader::RAY, piece->material.name.c_str());
            piece->program.attach(light_shader);
            piece->program.attach(vertex_shader);
            piece->program.link("light");
            rlUseProgram(piece->program.getProgram());
        }
        
        // Set the shader's uniforms.
        if (piece->material.component_flags.test(gfx::Material::DIFFUSE))
        {
            piece->program.set3fv("kd", piece->material.diffuse.v);
            
            if (piece->material.diffuse_texture.isValid())
            {
                piece->program.setTexture("diffuse_texture", piece->material.diffuse_texture.getTexture());
            }
            if (piece->material.normal_texture.isValid())
            {
            	piece->program.setTexture("normalmap", piece->material.normal_texture.getTexture());
            }
            if (piece->material.component_flags.test(gfx::Material::SUBSURFACE))
            {
                piece->program.set1i("subsurface", 1);
                piece->program.setPrimitive("subsurface_primitive", subsurface_prim);
            }
            
            // Attach the uniform buffer for the random data to this primitive. Also setup the bounce probability
            // for russian roulette for this particual material.
            RLuint gi_block_index = rlGetUniformBlockIndex(piece->program.getProgram(), "GI");
            if (gi_block_index != ~0) // All 0xF's returned if this block is invalid (this could be a light source).
            {
                rlUniformBlockBuffer(gi_block_index, gi_buffer.getBuffer());
                
                float bounce_probablility = (piece->material.diffuse[0] + piece->material.diffuse[1] + piece->material.diffuse[2]) / 3.0f;
                piece->program.set1f("bounce_probablility", bounce_probablility);
            }
        }
        if (piece->material.component_flags.test(gfx::Material::SPECULAR))
        {
            math::vec4f uniform(piece->material.specular);
            uniform[3] = piece->material.roughness;
            piece->program.set4fv("ks", uniform.v);
        }
        if (piece->material.component_flags.test(gfx::Material::TRANSMISSIVE))
        {
            math::vec4f uniform(piece->material.transmissive);
            uniform[3] = piece->material.index_of_refraction;
            piece->program.set4fv("kt", uniform.v);
        }
        
        // Attach the proper vbos to the attributes for this shader.
        gfx::Mesh::RenderData render_data;
        render_data.position_attribute  = rlGetAttribLocation(piece->program.getProgram(), "position_attribute");
        render_data.normal_attribute    = rlGetAttribLocation(piece->program.getProgram(), "normal_attribute");
        render_data.tex_coord_attribute = rlGetAttribLocation(piece->program.getProgram(), "tex_coord_attribute");
        render_data.tangent_attribute   = rlGetAttribLocation(piece->program.getProgram(), "tangent_attribute");
        
        piece->buffers[gfx::Mesh::VERTICES].bind();
        rlVertexAttribBuffer(render_data.position_attribute, 3, RL_FLOAT, RL_FALSE, sizeof(math::vec3f), 0);
        piece->buffers[gfx::Mesh::VERTICES].unbind();
        
        piece->buffers[gfx::Mesh::NORMALS].bind();
        rlVertexAttribBuffer(render_data.normal_attribute, 3, RL_FLOAT, RL_FALSE, sizeof(math::vec3f), 0);
        piece->buffers[gfx::Mesh::NORMALS].unbind();
        
        piece->buffers[gfx::Mesh::TEX_COORDS].bind();
        rlVertexAttribBuffer(render_data.tex_coord_attribute, 2, RL_FLOAT, RL_FALSE, sizeof(math::vec2f), 0);
        piece->buffers[gfx::Mesh::TEX_COORDS].unbind();
        
        piece->buffers[gfx::Mesh::TANGENTS].bind();
        rlVertexAttribBuffer(render_data.tangent_attribute, 3, RL_FLOAT, RL_FALSE, sizeof(math::vec3f), 0);
        piece->buffers[gfx::Mesh::TANGENTS].unbind();
        
        // Submit this mesh to OpenRL for heirarchy building and later on rendering.
        rlDrawArrays(RL_TRIANGLES, 0, piece->num_elements);
        
        rlBindPrimitive(RL_PRIMITIVE, RL_NULL_PRIMITIVE);
		if (util::checkRLErrors("Mesh::prepareForRendering() - Finished uploading a MeshPiece", true))
        {
            return false;
        }
    }
    
    return true;
}

/// Generate a ray shader based on a material.
bool ShaderGenerator::generateRayShader(const gfx::Material &material, gfx::Shader &shader) const
{
    // Start building the source string with the ray attributes.
    std::string source = "rayattribute vec3 color;\n"
                         "rayattribute bool is_diffuse_bounce;\n"
                         "uniform primitive defaultPrim;\n"
                         "varying vec3 normal;\n"
                         "varying vec2 tex_coords;\n"
                         "uniformblock Light { int count; primitive lights[5]; vec3 positions[5]; vec3 normals[5]; };\n"; // Allow at most 5 lights.
    
    // Figure out the number of rays to create in this shader and write the uniform
    // definitions for this shader.
    int num_rays = 0;
    if (material.component_flags.test(gfx::Material::SPECULAR))
    {
        num_rays += 1;
        generateSpecularCode(source);
    }
    if (material.component_flags.test(gfx::Material::TRANSMISSIVE))
    {
        num_rays += 2;
        
        // If we haven't added the specular code yet, add it now as the
        // transmissive code will utilize it.
        if (!material.component_flags.test(gfx::Material::SPECULAR))
        {
            generateSpecularCode(source);
        }
        
        generateTransmissiveCode(source);
    }
    if (material.component_flags.test(gfx::Material::DIFFUSE))
    {
		num_rays += 1; // Shadow ray counts are determined by the 'count' variable in the 'Light' uniform block. Add 1 for the diffuse bounce.
        num_rays += material.component_flags.test(gfx::Material::SUBSURFACE) ? 1 : 0; // Add a ray if this material uses subsurface scattering.
        generateDiffuseCode(source, material.diffuse_texture.isValid(), material.normal_texture.isValid());
    }
    
    assert((num_rays < 10) && "Unsuported ray count");
    
    // Write the setup shader.
    source += "void setup() { rl_OutputRayCount[0] = ";
    char ray_count = '0' + num_rays;
    source += ray_count;
    
    // If this is a diffuse shader, we must cast an extra ray for each light to determine if we're in shadow or not.
    if (material.component_flags.test(gfx::Material::DIFFUSE))
    {
        source += " + Light.count";
    }
    
    source += "; }\n";
    
    // Write the main definition.
    source += "void main()\n"
    	      "{\n";
    
    if (material.component_flags.test(gfx::Material::DIFFUSE))
    {
        // Call the diffuse code.
        source += "diffuse();\n";
    }
    
    if (material.component_flags.test(gfx::Material::SPECULAR))
    {
        // Call the specular code.
        source += "specular(rl_InRay.color * ks.xyz, normalize(normal));\n";
    }
    
    if (material.component_flags.test(gfx::Material::TRANSMISSIVE))
    {
        // Call the transmissive code.
        source += "transmissive();\n";
    }
    
    // TODO: Add support for specular and transmissive.
    
    // Finish the shader.
    source += "}\n";
    
    // Finally, create the shader from the assembled shader source string.
    if (!shader.createFromString(source, gfx::Shader::RAY, material.name))
    {
		return false;
    }
    
    return true;
}

/// Generate diffuse code.
void ShaderGenerator::generateDiffuseCode(std::string &source, bool use_texture, bool use_normalmap) const
{
    source += "varying vec3 tangent;\n";
    source += "varying vec3 bitangent;\n";
    
    if (use_normalmap)
    {
        source += "uniform sampler2D normalmap;\n"
                  "vec3 getNormal()\n"
                  "{\n"
                       // Convert the normal from tangent space to world space.
                  "    vec3 norm = texture2D(normalmap, tex_coords).zyx * 2.0 - 1.0;\n"
                  "    return mat3(normalize(tangent), normalize(bitangent), normalize(normal)) * norm;\n"
                  "}\n";
    }
    else
    {
        source += "vec3 getNormal() { return normalize(normal); }\n";
    }
    
    std::string tmp;
    util::readTextFile("Resources/shaders/diffuseBounce.rsh", tmp);
    source += tmp;
    
    if (!use_texture)
    {
        util::readTextFile("Resources/shaders/diffuse.rsh", tmp);
        source += tmp;
    }
    else
    {
        util::readTextFile("Resources/shaders/diffuseWithTexture.rsh", tmp);
        source += tmp;
    }
}

/// Generate specular code.
void ShaderGenerator::generateSpecularCode(std::string &source) const
{
    std::string tmp;
    util::readTextFile("Resources/shaders/specular.rsh", tmp);
    source += tmp;
}

/// Generate transmissive code.
void ShaderGenerator::generateTransmissiveCode(std::string &source) const
{
    std::string tmp;
    util::readTextFile("Resources/shaders/refraction.rsh", tmp);
    source += tmp;
}

