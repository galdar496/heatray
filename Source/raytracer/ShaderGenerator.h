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
    	bool generateShaders(gfx::Mesh   		&mesh,            //     IN: Mesh to generate shaders for.
                             gfx::Shader 		&vertex_shader,   //     IN: Vertex shader to link all ray shaders with.
                             gfx::Buffer 		&light_buffer,    //     IN: Buffer to bind to the light uniform block for each shader.
                             gfx::Buffer 		&gi_buffer,       //     IN: Buffer to bind to diffuse objects which contains GI information.
    						 std::vector<Light> &lights 		  // IN/OUT: Information about the lights. This function will generate primitives and programs that the lights will use.
                            );
    
    private:
    
        /// Generate a ray shader based on a material.
        bool generateRayShader(const gfx::Material &material, //  IN: Material definition to generate the ray shader from.
                               gfx::Shader &shader            // OUT: Generated ray shader.
                              ) const;
    
    	/// Generate diffuse code.
    	void generateDiffuseCode(std::string &source, // IN/OUT: Current shader source. After execution the diffuse code will be added.
                                 bool use_texture,    //     IN: If true, generate texture sampling code as well.
    						     bool use_normalmap   //     IN: If true, use a normalmap to get the shading normal.
                                ) const;
    
        /// Generate specular code.
        void generateSpecularCode(std::string &source // IN/OUT: Current shader source. After execution the specular code will be added.
                                 ) const;
        
        /// Generate transmissive code.
        void generateTransmissiveCode(std::string &source // IN/OUT: Current shader source. After execution the transmissive code will be added.
        							 ) const;
};