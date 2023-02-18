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
#include <memory>

namespace openrl {

//
// Encapsulates an RLSL program.
//

class Program
{
public:
    ~Program() {
        if (m_program != RL_NULL_PROGRAM) {
#ifdef __APPLE__
            // This is pretty stupid but something about the arm-based Mac emulating
            // x86_64 conflicts with the OpenRL driver attempting to delete a program.
            // It doesn't always fail, maybe 1 out of 4 times but when it does it locks up
            // the app and it can't even be forced to quit (have to restart the machine).
            // Yes this leaks memory but that's better than locking up the system for the
            // time being.
#else
            RLFunc(rlDeleteProgram(m_program));
#endif // __APPLE__
            m_program = RL_NULL_PROGRAM;
        }
    }
    Program(const Program& other) = default;
    Program& operator=(const Program& other) = default;
    
    //-------------------------------------------------------------------------
    // Create the internal OpenRL objects for a Program and return a shared_ptr.
    static std::shared_ptr<Program> create() {
        return std::shared_ptr<Program>(new Program);
    }

    //-------------------------------------------------------------------------
    // Attach the specified shader. The shader must already be valid.
    inline void attach(const std::shared_ptr<Shader> shader)
    {
        assert(shader->valid());
        RLFunc(rlAttachShader(m_program, shader->shader()));
        m_attachedShaders[static_cast<uint8_t>(shader->type())] = shader;
    }

    //-------------------------------------------------------------------------
    // Link the program. This must be called after all shaders have been attached
    // to finish program creation.
    inline bool link(const char* name = "Program")
    {
        RLFunc(rlLinkProgram(m_program));

        if (!valid()) {
            const char* log = nullptr;
            RLFunc(rlGetProgramString(m_program, RL_LINK_LOG, &log));
            LOG_ERROR("Linking for program %s failed: \n\t%s", name, log);
            return false;
        }

        return true;
    }

    //-------------------------------------------------------------------------
    // Get the location of a uniform by name.
    inline RLint getUniformLocation(const std::string& name) const
    {
        RLint location = RLFunc(rlGetUniformLocation(m_program, name.c_str()));
        return location;
    }

    //-------------------------------------------------------------------------
    // Get the index of a uniform block by name.
    inline RLint getUniformBlockIndex(const std::string& name) const
    {
        RLint index = RLFunc(rlGetUniformBlockIndex(m_program, name.c_str()));
        return index;
    }

    //-------------------------------------------------------------------------
    // Set a uniform block at a particular index.
    inline void setUniformBlock(const RLint blockIndex, const RLbuffer buffer) const
    {
        assert(blockIndex != -1);
        assert(buffer != RL_NULL_BUFFER);
        RLFunc(rlUniformBlockBuffer(blockIndex, buffer));
    }

    //-------------------------------------------------------------------------
    // Various uniform setters.
    inline void set1i(const RLint location, const int i) const { RLFunc(rlUniform1i(location, i)); }
    inline void set1f(const RLint location, const float f) const { RLFunc(rlUniform1f(location, f)); }
    inline void set2fv(const RLint location, const float* f) const { RLFunc(rlUniform2fv(location, 1, f)); }
    inline void set2iv(const RLint location, const int* i) const { RLFunc(rlUniform2iv(location, 1, i)); }
    inline void set3fv(const RLint location, const float* f) const { RLFunc(rlUniform3fv(location, 1, f)); }
    inline void set4fv(const RLint location, const float* f) const { RLFunc(rlUniform4fv(location, 1, f)); }
    inline void set4iv(const RLint location, const int* i) const { RLFunc(rlUniform4iv(location, 1, i)); }
    inline void setMatrix4fv(const RLint location, const float* f) const { RLFunc(rlUniformMatrix4fv(location, 1, RL_FALSE, f)); }
    inline void setTexture(const RLint location, const std::shared_ptr<Texture> texture) const 
    {
        assert(texture->valid());
        RLFunc(rlUniformt(location, texture->texture())); 
    }
    inline void setPrimitive(const RLint location, const RLprimitive& primitive) const
    { 
        //TODO: There is a circular include dependency if this function takes openrl::Primitive. Need to solve that.

        RLFunc(rlUniformp(location, primitive)); 
    }

    //-------------------------------------------------------------------------
    // Bind this program for use.
    inline void bind() const { RLFunc(rlUseProgram(m_program)); }

    //-------------------------------------------------------------------------
    // Unbind this program from use.
    inline void unbind() const { RLFunc(rlUseProgram(RL_NULL_PROGRAM)); }

    //-------------------------------------------------------------------------
    // Get a location of an attribute variable in the program.
    inline RLint getAttributeLocation(const std::string& name) const { return RLFunc(rlGetAttribLocation(m_program, name.c_str())); }

    //-------------------------------------------------------------------------
    // Access to the internal program objects.
    inline RLprogram program() const { return m_program; }

    //-------------------------------------------------------------------------
    // Determine if the program is valid or not for use by OpenRL.
    inline bool valid() const 
    { 
        RLint status = -1;
        RLFunc(rlGetProgramiv(m_program, RL_LINK_STATUS, &status));
        return status != 0; 
    }

private:
    Program() {
        m_program = RLFunc(rlCreateProgram());
    }

    RLprogram m_program = RL_NULL_PROGRAM; // RL program object.

    // We allow for one attached shader per shader type.
    std::shared_ptr<Shader> m_attachedShaders[static_cast<uint8_t>(Shader::ShaderType::kCount)];
};

} // namespace openrl.

