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

///
/// Class to auto-generate shaders based a given mesh.
///

class ShaderGenerator
{
    public:
    
        ShaderGenerator();
    	~ShaderGenerator();
    
        struct GenerationInfo
        {
            gfx::Mesh   *mesh;           ///< Mesh to generate all shaders for. A shader is generated per-material.
            gfx::Shader *vertexShader;   ///< Vertex shader to link all ray shaders with.
            gfx::Buffer *lightBuffer;    ///< Buffer to bind to the light uniform block for each shader.
            gfx::Buffer *giBuffer;       ///< Buffer to bind to diffuse objects which contains GI information.
            std::string rayShaderPath;   ///< Relative path to the ray shader source file.
            std::string lightShaderPath; ///< Relative path to the light shader source file.
            int         maxLightCount;   ///< Maximum number of lights supported.
            
            std::vector<Light> *lights; ///< Information about the lights. This value will be updated after a call to GenerateShaders().
        };
    
        ///
        /// Generate all of the necessary shaders for the current mesh. After a shader has been generated,
        /// it is automatically setup and bound to the proper mesh piece.
        ///
        /// @param info A GenerationInfo object that specifies necessary info for generating the shaders.
        ///
        /// @return If true, the shaders were all properly generated.
    	bool GenerateShaders(const GenerationInfo &info);
    
    private:
    
        ///
        /// Generate a ray shader based on a material OR find the existing shader in the shader cache. If a ray shader was not found and
        /// count not be created, null is returned.
        ///
        /// @param material Material definition to generate the ray shader from.
        /// @param shaderSource Shader source to use to generate a shader.
        /// @param maxLightCount Maximum number of lights supported.
        ///
        /// @return A shader to use for binding to a program. If the shader could not be create, null is returned.
        ///
        gfx::Shader *FindOrCreateRayShader(const gfx::Material &material, const std::string &shaderSource, const int maxLightCount);

        typedef unsigned int ShaderFlags;
        typedef std::unordered_map<ShaderFlags, gfx::Shader *> ShaderCache;
        ShaderCache m_shaderCache; ///< Cache of the generated shaders.
};