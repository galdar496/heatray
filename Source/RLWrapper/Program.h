//
//  Program.h
//  Heatray
//
//  
//
//

#pragma once

#include "Error.h"
#include "Shader.h"
#include "Texture.h"

#include "../Utility/Log.h"

#include <OpenRL/rl.h>
#include <assert.h>

namespace openrl {

///
/// Encapsulates an RLSL program.
///

class Program
{
public:

    Program() = default;
    Program(const Program& other) = default;
    Program& operator=(const Program& other) = default;
    ~Program() = default;

    ///
    /// Create the RL program.
    ///
    inline void create()
    {
        if (m_program == RL_NULL_PROGRAM) {
            m_program = RLFunc(rlCreateProgram());
        }
    }

    ///
    /// Attach the specified shader. The shader must already be valid.
    ///
    /// @param shader Shader to attach to this program. This shader must have already been compiled.
    ///
    /// @return If true, the shader was properly attached.
    ///
    inline void attach(const Shader& shader)
    {
        assert(shader.valid());
        RLFunc(rlAttachShader(m_program, shader.shader()));
    }

    ///
    /// Link the program. Call this after all shaders have been attached to finish program creation.
    ///
    /// @param name Optional parameter to name the program for debug output.
    ///
    inline bool link(const char* name = "Program")
    {
        RLFunc(rlLinkProgram(m_program));

        if (!valid()) {
            const char* log = nullptr;
            RLFunc(rlGetProgramString(m_program, RL_LINK_LOG, &log));
			LOG_INFO("Linking for program %s failed: \n\t%s", name, log);
            return false;
        }

        return true;
    }

    ///
    /// Destroy this program.
    ///
    inline void destroy()
    {
        if (m_program != RL_NULL_PROGRAM) {
            RLFunc(rlDeleteProgram(m_program));
            m_program = RL_NULL_PROGRAM;
        }
    }

    ///
    /// Get the location of a uniform by name.
    ///
    inline RLint getUniformLocation(const std::string& name) const
    {
        RLint location = RLFunc(rlGetUniformLocation(m_program, name.c_str()));
        return location;
    }

    ///
    /// Get the index of a uniform block by name.
    ///
    inline RLint getUniformBlockIndex(const std::string& name) const
    {
        RLint index = RLFunc(rlGetUniformBlockIndex(m_program, name.c_str()));
        return index;

    }

    ///
    /// Set a uniform block at a particular index.
    ///
    inline void setUniformBlock(const RLint blockIndex, const RLbuffer buffer) const
    {
        assert(blockIndex != -1);
        assert(buffer != RL_NULL_BUFFER);
        RLFunc(rlUniformBlockBuffer(blockIndex, buffer));
    }

    ///
    /// Set an integer uniform in the program.
    ///
    /// @param location Location of this uniform in the shader.
    /// @param i Value of the uniform.
    ///
    inline void set1i(const RLint location, const int i) const { RLFunc(rlUniform1i(location, i)); }

    ///
    /// Set a float uniform in the program.
    ///
    /// @param location Location of this uniform in the shader.
    /// @param f Value of the uniform.
    ///
    inline void set1f(const RLint location, const float f) const { RLFunc(rlUniform1f(location, f)); }

    ///
    /// Set a 2 component float uniform in the program.
    ///
    /// @param location Location of this uniform in the shader.
    /// @param f Value of the uniform.
    ///
    inline void set2fv(const RLint location, const float* f) const { RLFunc(rlUniform2fv(location, 1, f)); }

    ///
    /// Set a 2 component int uniform in the program.
    ///
    /// @param location Location of this uniform in the shader.
    /// @param i Value of the uniform.
    ///
    inline void set2iv(const RLint location, const int* i) const { RLFunc(rlUniform2iv(location, 1, i)); }

    ///
    /// Set a 3 component float uniform in the program.
    ///
    /// @param location Location of this uniform in the shader.
    /// @param f Value of the uniform.
    ///
    inline void set3fv(const RLint location, const float* f) const { RLFunc(rlUniform3fv(location, 1, f)); }

    ///
    /// Set a 4 component float uniform in the program.
    ///
    /// @param location Location of this uniform in the shader.
    /// @param f Value of the uniform.
    ///
    inline void set4fv(const RLint location, const float* f) const { RLFunc(rlUniform4fv(location, 1, f)); }

    ///
    /// Set a 4 component int uniform in the program.
    ///
    /// @param location Location of this uniform in the shader.
    /// @param fiValue of the uniform.
    ///
    inline void set4iv(const RLint location, const int* i) const { RLFunc(rlUniform4iv(location, 1, i)); }

    ///
    /// Set a 4x4 matrix uniform in the program.
    ///
    /// @param location Location of this uniform in the shader.
    /// @param f Value of the uniform.
    ///
    inline void setMatrix4fv(const RLint location, const float* f) const { RLFunc(rlUniformMatrix4fv(location, 1, RL_FALSE, f)); }

    ///
    /// Set a texture uniform in the program.
    ///
    /// @param location Location of this uniform in the shader.
    /// @param texture Value of the uniform.
    ///
    inline void setTexture(const RLint location, const Texture& texture) const 
    {
        assert(texture.valid());
        RLFunc(rlUniformt(location, texture.texture())); 
    }

    ///
    /// Set a primitive uniform in the program.
    ///
    /// @param location Location of this uniform in the shader.
    /// @param primitive Value of the uniform.
    ///
    inline void setPrimitive(const RLint location, const RLprimitive& primitive) const
    { 
        //TODO: There is a circular include dependency if this function takes openrl::Primitive. Need to solve that.

        RLFunc(rlUniformp(location, primitive)); 
    }

    ///
    /// Bind this program for use.
    ///
    inline void bind() const { RLFunc(rlUseProgram(m_program)); }

    ///
    /// Unbind this program from use.
    ///
    inline void unbind() const { RLFunc(rlUseProgram(RL_NULL_PROGRAM)); }

    ///
    /// Get a location of an attribute variable in the program.
    ///
    /// @param name Name of the attribute variable.
    ///
    inline RLint getAttributeLocation(const std::string& name) const { return RLFunc(rlGetAttribLocation(m_program, name.c_str())); }

    ///
    /// Get access to the internal program object.
    ///
    inline RLprogram program() const { return m_program; }

    inline bool valid() const 
    { 
        RLint status = -1;
        RLFunc(rlGetProgramiv(m_program, RL_LINK_STATUS, &status));
        return status != 0; 
    }

private:

    RLprogram m_program = RL_NULL_PROGRAM; ///< RL program object.
};

} // namespace openrl.

