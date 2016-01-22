//
//  Shader.cpp
//  Heatray
//
//  Created by Cody White on 5/9/14.
//
//

#include "Shader.h"
#include "../utility/util.h"
#include <assert.h>
#include <iostream>

namespace gfx
{

    /// Default constructor.
Shader::Shader() :
    m_shader(RL_NULL_SHADER),
    m_name("<unnamed>")
{
    
}

/// Destructor.
Shader::~Shader()
{
    if (m_shader != RL_NULL_SHADER)
    {
        rlDeleteShader(m_shader);
        m_shader = RL_NULL_SHADER;
    }
}

/// Load a shader from a filepath and compile it for the specified shader type.
/// Returns true if the shader is loaded and compiled successfully.
bool Shader::load(const std::string &path, const Type type)
{
    std::string source;
    if (!util::ReadTextFile(path, source))
    {
        return false;
    }
    
    return createFromString(source, type, path);
}
    
/// Create and compile a shader from a passed-in source string.
bool Shader::createFromString(const std::string &shaderSource, const Type type, const std::string &name)
{
	bool returnValue = false;
    if (shaderSource.length() && createShader(type))
    {
        rlShaderString(m_shader, RL_SHADER_NAME, name.c_str());
        
		const char *s = shaderSource.c_str();
        rlShaderSource(m_shader, 1, &s, NULL);

        m_name = name;
        returnValue = compile();
    }
    
    return returnValue;
}
    
/// Destroy this shader.
void Shader::destroy()
{
    if (m_shader != RL_NULL_SHADER)
    {
        rlDeleteShader(m_shader);
        m_shader = RL_NULL_SHADER;
    }
}
    
/// Get access to the internal RL shader object.
RLshader Shader::getShader() const
{
	return m_shader;
}

/// Compile the shader for use. Returns true on a successful compile.
bool Shader::compile() const
{
	rlCompileShader(m_shader);
	util::CheckRLErrors("Shader::compile() - compiling shader", true);
    
    RLint success = 0;
	rlGetShaderiv(m_shader, RL_COMPILE_STATUS, &success);
    
    if (success != RL_TRUE)
    {
        const char *log = NULL;
        rlGetShaderString(m_shader, RL_COMPILE_LOG, &log);
        std::cout << "Shader::compile() -- Unable to compile shader " << m_name << "\n\t" << log << std::endl;
        return false;
    }
    
	return true;
}

/// Create the shader. Returns true on success.
bool Shader::createShader(const Type type)
{
    RLenum shader_type = ~0;
    
    switch (type)
    {
        case VERTEX:
            shader_type = RL_VERTEX_SHADER;
            break;
            
        case FRAME:
            shader_type = RL_FRAME_SHADER;
            break;
            
        case RAY:
            shader_type = RL_RAY_SHADER;
            break;
            
        case PREFIX:
            shader_type = RL_PREFIX_RAY_SHADER;
            break;
            
        default:
            assert(0 && "Trying to create unknown shader type");
			return false;
    }
    
    m_shader = rlCreateShader(shader_type);
    return !util::CheckRLErrors("Shader::createShader() -- shader created", true);
}
    
} // namsespace gfx