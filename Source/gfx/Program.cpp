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

namespace gfx
{

/// Default constructor.
Program::Program() :
	m_program(RL_NULL_PROGRAM),
	m_program_name("<unnamed>")
{
}

/// Destructor.
Program::~Program()
{
    destroy();
}

/// Attach the specified shader. The shader must already be valid.
bool Program::attach(const Shader &shader)
{
	bool returnValue = false;
    
    if (create())
    {
        rlAttachShader(m_program, shader.getShader());
        
        CheckRLErrors();
        returnValue = true;
    }
    
    return returnValue;
}

/// Link the program. Call this after all shaders have been attached.
bool Program::link(const std::string &name)
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
    
	m_program_name = name;
	return true;
}

/// Add a shader directly to the program.
bool Program::addShader(const std::string &filename, const Shader::Type type)
{
	Shader shader;
	if (shader.load(filename, type) == false)
	{
		return false;
	}
    
	if (attach(shader) == false)
	{
		return false;
	}
    
	return true;
}
    
/// Destroy this program.
void Program::destroy()
{
    if (m_program != RL_NULL_PROGRAM)
    {
        rlDeleteProgram(m_program);
        m_program = RL_NULL_PROGRAM;
    }
}

/// Set an integer uniform in the program.
void Program::set1i(const std::string &name, const int i) const
{
    rlUniform1i(rlGetUniformLocation(m_program, name.c_str()), i);
}

/// Set a float uniform in the program.
void Program::set1f(const std::string &name, const float f) const
{
    rlUniform1f(rlGetUniformLocation(m_program, name.c_str()), f);
}

/// Set a 2 component float uniform in the program.
void Program::set2fv(const std::string &name, const float *f) const
{
	rlUniform2fv(rlGetUniformLocation(m_program, name.c_str()), 1, f);
}

/// Set a 2 component int uniform in the program.
void Program::set2iv(const std::string &name, const int *i) const
{
	rlUniform2iv(rlGetUniformLocation(m_program, name.c_str()), 1, i);
}

/// Set a 3 component float uniform in the program.
void Program::set3fv(const std::string &name, const float *f) const
{
    rlUniform3fv(rlGetUniformLocation(m_program, name.c_str()), 1, f);
}

/// Set a 4 component int uniform in the program.
void Program::set4iv(const std::string &name, const int *i) const
{
    rlUniform4iv(rlGetUniformLocation(m_program, name.c_str()), 1, i);
}
    
/// Set a 4 component float uniform in the program.
void Program::set4fv(const std::string &name, const float *f) const
{
    rlUniform4fv(rlGetUniformLocation(m_program, name.c_str()), 1, f);
}

/// Set a 4x4 matrix uniform in the program.
void Program::setMatrix4fv(const std::string &name, const float *f) const
{
    rlUniformMatrix4fv(rlGetUniformLocation(m_program, name.c_str()), 1, RL_FALSE, f);
}
    
/// Set a texture uniform in the program.
void Program::setTexture(const std::string &name, const RLtexture &texture)
{
	rlUniformt(rlGetUniformLocation(m_program, name.c_str()), texture);
}
    
/// Set a primitive uniform in the program.
void Program::setPrimitive(const std::string &name, const RLprimitive &primitive)
{
    rlUniformp(rlGetUniformLocation(m_program, name.c_str()), primitive);
}

/// Bind this program for use.
void Program::bind() const
{
    rlUseProgram(m_program);
}

/// Unbind this program from use.
void Program::unbind() const
{
	rlUseProgram(RL_NULL_PROGRAM);
}

/// Get a location of an attribute variable in the program.
RLint Program::getAttributeLocation(const std::string &name) const
{
    return rlGetAttribLocation(m_program, name.c_str());
}
    
/// Get access to the internal program object.
RLprogram Program::getProgram() const
{
    return m_program;
}
    
bool Program::create()
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


