//
//  ShaderGenerator.h
//  Heatray
//
//  Created by Cody White on 5/28/14.
//
//

#pragma once

#include "../gfx/Mesh.h"
#include "../gfx/Shader.h"
#include "../gfx/Buffer.h"
#include "Light.h"
#include <string>
#include <unordered_map>

/// Class to auto-generate shaders based a given mesh.
class ShaderGenerator
{
    public:
    
    	/// Default constructor.
        ShaderGenerator();
    
    	/// Destructor.
    	~ShaderGenerator();
    
    	/// Generate all of the necessary shaders for the current mesh. After a shader has been generated,
        /// it is automatically setup and bound to the proper mesh piece.
    	bool generateShaders(gfx::Mesh   		&mesh,             //     IN: Mesh to generate shaders for.
                             gfx::Shader 		&vertex_shader,    //     IN: Vertex shader to link all ray shaders with.
                             gfx::Buffer 		&light_buffer,     //     IN: Buffer to bind to the light uniform block for each shader.
                             gfx::Buffer 		&gi_buffer,        //     IN: Buffer to bind to diffuse objects which contains GI information.
                             const std::string &ray_shader_path,   //     IN: Path to the ray shader source file.
                             const std::string &light_shader_path, //     IN: Path to the light shader source file.
                             const int          max_light_count,   //     IN: Maximum number of lights supported.
    						 std::vector<Light> &lights  		   // IN/OUT: Information about the lights. This function will generate programs that the lights will use.
                            );
    
    private:
    
        /// Generate a ray shader based on a material OR find the existing shader in the shader cache. If a ray shader was not found and
        /// count not be created, null is returned.
        gfx::Shader *findOrCreateRayShader(const gfx::Material &material,    // IN: Material definition to generate the ray shader from.
                                           const std::string &shader_source, // IN: Shader source to use to generate a shader.
                                           const int max_light_count         // IN: Maximum number of lights supported.
                                          );

        typedef unsigned int ShaderFlags;
        typedef std::unordered_map<ShaderFlags, gfx::Shader *> ShaderCache;
        ShaderCache m_shaderCache; // Cache of the generated shaders.
};