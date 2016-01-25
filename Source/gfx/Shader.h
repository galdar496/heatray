//
//  Shader.h
//  Heatray
//
//  Created by Cody White on 5/9/14.
//
//

#pragma once

#include <OpenRL/rl.h>
#include <string>

namespace gfx
{

///
/// Class to contain an OpenRL shader. Handles shader loading and compilation.
///

class Shader
{
    public:
    
        ///
    	/// Possible shader types.
        ///
    	enum class ShaderType
        {
            kVertex,
            kFrame,
            kRay,
            kPrefix,
        };
    
    	Shader();
    	~Shader();
    
        ///
    	/// Load a shader from a filepath and compile it for the specified shader type.
        ///
        /// @param path Absolute path to the shader file to load.
        /// @param type Type of the shader to compile as defined by the enum "ShaderType".
        ///
        /// @return Returns true if the shader is loaded and compiled successfully.
        ///
    	bool Load(const std::string &path, const ShaderType type);
    
        ///
    	/// Create and compile a shader from a passed-in source string.
        ///
        /// @param shaderSource Source code for the shader.
        /// @param type Type of the shader to compile as defined by the enum "ShaderType".
        /// @param name Name to use for shader identification.
        ///
        /// @param If true, the shader was successfully created.
        ///
    	bool CreateFromString(const std::string &shaderSource, const ShaderType type, const std::string &name);
    
        ///
        /// Destroy this shader.
        ///
        void Destroy();
    
        ///
    	/// Get access to the internal RL shader object.
        ///
    	RLshader GetShader() const;
    
        ///
        /// Check the status of this shader.
        ///
        /// @return If true, this shader is considered to be valid and ready to be
        ///         attached to a program.
        ///
        bool IsValid() const;
    
    private:
    
        ///
        /// Compile the shader for use. Returns true on a successful compile.
        ///
        bool Compile() const;
    
        ///
        /// Create the shader.
        ///
        /// @param type Type of the shader to compile.
        ///
        /// @return If true, the shader was successfully created.
        ///
        bool CreateShader(const ShaderType type);
    
    	// Shaders are not copyable.
    	Shader(const Shader &other) = delete;
    	Shader & operator=(const Shader &other) = delete;
    
    	// Member variables.
    	RLshader    m_shader; ///< OpenRL shader configured by this class.
    	std::string m_name;   ///< Name of the shader.
};

} // namespace gfx