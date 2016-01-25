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

Shader::Shader() :
    m_shader(RL_NULL_SHADER),
    m_name("<unnamed>")
{
    
}

Shader::~Shader()
{
    if (m_shader != RL_NULL_SHADER)
    {
        rlDeleteShader(m_shader);
        m_shader = RL_NULL_SHADER;
    }
}

bool Shader::Load(const std::string &path, const ShaderType type)
{
    std::string source;
    if (!util::ReadTextFile(path, source))
    {
        return false;
    }
    
    return CreateFromString(source, type, path);
}
    
bool Shader::CreateFromString(const std::string &shaderSource, const ShaderType type, const std::string &name)
{
	bool returnValue = false;
    if (shaderSource.length() && CreateShader(type))
    {
        rlShaderString(m_shader, RL_SHADER_NAME, name.c_str());
        
		const char *s = shaderSource.c_str();
        rlShaderSource(m_shader, 1, &s, nullptr);

        m_name = name;
        returnValue = Compile();
    }
    
    return returnValue;
}
    
void Shader::Destroy()
{
    if (m_shader != RL_NULL_SHADER)
    {
        rlDeleteShader(m_shader);
        m_shader = RL_NULL_SHADER;
    }
}
    
RLshader Shader::GetShader() const
{
	return m_shader;
}

bool Shader::IsValid() const
{
    RLint success = 0;
    rlGetShaderiv(m_shader, RL_COMPILE_STATUS, &success);
    return (success == RL_TRUE);
}

bool Shader::Compile() const
{
	rlCompileShader(m_shader);
    CheckRLErrors();
    
    if (!IsValid())
    {
        const char *log = nullptr;
        rlGetShaderString(m_shader, RL_COMPILE_LOG, &log);
        std::cout << "Shader::compile() -- Unable to compile shader " << m_name << "\n\t" << log << std::endl;
        return false;
    }
    
	return true;
}

bool Shader::CreateShader(const ShaderType type)
{
    RLenum shaderType = ~0;
    
    switch (type)
    {
        case ShaderType::kVertex:
            shaderType = RL_VERTEX_SHADER;
            break;
            
        case ShaderType::kFrame:
            shaderType = RL_FRAME_SHADER;
            break;
            
        case ShaderType::kRay:
            shaderType = RL_RAY_SHADER;
            break;
            
        case ShaderType::kPrefix:
            shaderType = RL_PREFIX_RAY_SHADER;
            break;
            
        default:
            assert(0 && "Trying to create unknown shader type");
			return false;
    }
    
    m_shader = rlCreateShader(shaderType);
    CheckRLErrors();
    return true;
}
    
} // namsespace gfx