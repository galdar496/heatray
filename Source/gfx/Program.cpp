//
//  Program.cpp
//  Heatray
//
//  Created by Cody White on 5/9/14.
//
//

#include "Program.h"
#include "../utility/util.h"
#include <iostream>
#include <assert.h>

namespace gfx
{

Program::Program() :
	m_program(RL_NULL_PROGRAM),
	m_programName("<unnamed>")
{
}

Program::~Program()
{
    Destroy();
}

bool Program::Attach(const Shader &shader)
{
	bool returnValue = false;
    assert(shader.IsValid());
    
    if (Create())
    {
        rlAttachShader(m_program, shader.getShader());
        
        CheckRLErrors();
        returnValue = true;
    }
    
    return returnValue;
}

bool Program::Link(const std::string &name)
{
	rlLinkProgram(m_program);
	//util::CheckRLErrors("Program::link() - Link the program", true);
    
    RLint success = -1;
	rlGetProgramiv(m_program, RL_LINK_STATUS, &success);
    
	if (!success)
	{
        const char *log = NULL;
        rlGetProgramString(m_program, RL_LINK_LOG, &log);
        std::cout << "Linking for program " << name << " failed: \n\t" << log << std::endl;
        return false;
	}
    
	m_programName = name;
	return true;
}

bool Program::AddShader(const std::string &filename, const Shader::Type type)
{
	Shader shader;
	if (shader.load(filename, type) == false)
	{
		return false;
	}
    
	if (Attach(shader) == false)
	{
		return false;
	}
    
	return true;
}
    
void Program::Destroy()
{
    if (m_program != RL_NULL_PROGRAM)
    {
        rlDeleteProgram(m_program);
        m_program = RL_NULL_PROGRAM;
    }
}

RLint Program::GetUniformLocation(const std::string &name) const
{
    RLint location = rlGetUniformLocation(m_program, name.c_str());
    
    assert(location >= 0);
    return location;
}

void Program::Set1i(const RLint location, const int i) const
{
    rlUniform1i(location, i);
}

void Program::Set1f(const RLint location, const float f) const
{
    rlUniform1f(location, f);
}

void Program::Set2fv(const RLint location, const float *f) const
{
	rlUniform2fv(location, 1, f);
}

void Program::Set2iv(const RLint location, const int *i) const
{
	rlUniform2iv(location, 1, i);
}

void Program::Set3fv(const RLint location, const float *f) const
{
    rlUniform3fv(location, 1, f);
}

void Program::Set4iv(const RLint location, const int *i) const
{
    rlUniform4iv(location, 1, i);
}
    
void Program::Set4fv(const RLint location, const float *f) const
{
    rlUniform4fv(location, 1, f);
}

void Program::SetMatrix4fv(const RLint location, const float *f) const
{
    rlUniformMatrix4fv(location, 1, RL_FALSE, f);
}
    
void Program::SetTexture(const RLint location, const RLtexture &texture)
{
	rlUniformt(location, texture);
}
    
void Program::SetPrimitive(const RLint location, const RLprimitive &primitive)
{
    rlUniformp(location, primitive);
}

void Program::Bind() const
{
    rlUseProgram(m_program);
}

void Program::Unbind() const
{
	rlUseProgram(RL_NULL_PROGRAM);
}

RLint Program::GetAttributeLocation(const std::string &name) const
{
    return rlGetAttribLocation(m_program, name.c_str());
}
    
RLprogram Program::GetProgram() const
{
    return m_program;
}
    
bool Program::Create()
{
    if (m_program != RL_NULL_PROGRAM)
    {
        // Program has already been created.
        return true;
    }
    
	m_program = rlCreateProgram();
    return (m_program != RL_NULL_PROGRAM);
}
    
} // namespace gfx


