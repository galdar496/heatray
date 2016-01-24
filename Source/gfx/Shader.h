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

/// Class to contain an OpenRL shader. Handles shader loading and compilation.
class Shader
{
    public:
    
    	// Possible shader types.
    	enum Type
        {
            VERTEX,
            FRAME,
            RAY,
            PREFIX,
        };
    
    	/// Default constructor.
    	Shader();
    
    	/// Destructor.
    	~Shader();
    
    	/// Load a shader from a filepath and compile it for the specified shader type.
        /// Returns true if the shader is loaded and compiled successfully.
    	bool load(const std::string &path,  // IN: Absolute path to the shader file to load.
    			  const Type type			// IN: Type of the shader to compile as defined by the enum "Type".
                  );
    
    	/// Create and compile a shader from a passed-in source string.
    	bool createFromString(const std::string &shaderSource, // IN: Source code for the shader.
    						  const Type type,				   // IN: Type of the shader to compile as defined by the enum "Type".
    						  const std::string &name          // IN: Name to use for shader identification.
                             );
    
        /// Destroy this shader.
        void destroy();
    
    	/// Get access to the internal RL shader object.
    	RLshader getShader() const;
    
        ///
        /// Check the status of this shader.
        ///
        /// @return If true, this shader is considered to be valid and ready to be
        ///         attached to a program.
        ///
        bool IsValid() const;
    
    private:
    
        /// Compile the shader for use. Returns true on a successful compile.
        bool compile() const;
    
        /// Create the shader. Returns true on success.
        bool createShader(const Type type // IN: Type of the shader to compile.
                         );
    
    	// Shaders are not copyable.
    	Shader(const Shader &other);
    	Shader & operator=(const Shader &other);
    
    	// Member variables.
    	RLshader    m_shader; // OpenRL shader configured by this class.
    	std::string m_name;   // Name of the shader.
};

} // namespace gfx