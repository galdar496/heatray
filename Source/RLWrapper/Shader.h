//
//  Shader.h
//  Heatray
//
//  C++ wrapper for an OpenRL shader.
//
//

#pragma once

#include "Error.h"

#include <OpenRL/rl.h>
#include <assert.h>
#include <iostream>
#include <string>
#include <vector>

namespace openrl
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
    enum class ShaderType : uint8_t
    {
        kVertex,
        kFrame,
        kRay,
        kPrefix,
    };

    Shader() = default;
    Shader(const Shader& other) = default;
    Shader& operator=(const Shader& other) = default;
    ~Shader() = default;

    ///
    /// Create and compile a shader from a passed-in source string.
    ///
    /// @param shaderSource Source code for the shader.
    /// @param type Type of the shader to compile as defined by the enum "ShaderType".
    /// @param name Name to use for shader identification.
    ///
    /// @param If true, the shader was successfully created.
    ///
    inline bool createFromString(const std::string& shaderSource, const RLenum type, const char* name)
    {
        if (shaderSource.length() && createShader(type))
        {
            RLFunc(rlShaderString(m_shader, RL_SHADER_NAME, name));

            const char* s = shaderSource.c_str();
            RLFunc(rlShaderSource(m_shader, 1, &s, nullptr));

            if (compile())
            {
                return true;
            }
            else
            {
                const char* log = nullptr;
                RLFunc(rlGetShaderString(m_shader, RL_COMPILE_LOG, &log));
                std::cout << "Shader::createFromString() -- Unable to compile shader " << name << "\n\t" << log << std::endl;
                return false;
            }
        }

        return false;;
    }

    inline bool createFromMultipleStrings(const std::vector<std::string>& shaderSource, const RLenum type, const char* name)
    {
        if (shaderSource.size() && createShader(type))
        {
            RLFunc(rlShaderString(m_shader, RL_SHADER_NAME, name));

            char** strings = new char* [shaderSource.size()];
            for (int iIndex = 0; iIndex < shaderSource.size(); ++iIndex)
            {
                assert(shaderSource[iIndex].length());
                strings[iIndex] = new char[shaderSource[iIndex].length() + 1];
                strcpy(strings[iIndex], shaderSource[iIndex].c_str());
            }

            RLFunc(rlShaderSource(m_shader, shaderSource.size(), strings, nullptr));

            for (int iIndex = 0; iIndex < shaderSource.size(); ++iIndex)
            {
                delete[] strings[iIndex];
            }
            delete[] strings;

            if (compile())
            {
                return true;
            }
            else
            {
                const char* log = nullptr;
                RLFunc(rlGetShaderString(m_shader, RL_COMPILE_LOG, &log));
                std::cout << "Shader::createFromString() -- Unable to compile shader " << name << "\n\t" << log << std::endl;
                return false;
            }
        }

        return false;;
    }

    ///
    /// Destroy this shader.
    ///
    inline void destroy()
    {
        if (m_shader != RL_NULL_SHADER)
        {
            RLFunc(rlDeleteShader(m_shader));
            m_shader = RL_NULL_SHADER;
        }
    }

    ///
    /// Get access to the internal RL shader object.
    ///
    inline RLshader shader() const { return m_shader; }

    ///
    /// Check the status of this shader.
    ///
    /// @return If true, this shader is considered to be valid and ready to be
    ///         attached to a program.
    ///
    inline bool valid() const
    {
        RLint success = 0;
        RLFunc(rlGetShaderiv(m_shader, RL_COMPILE_STATUS, &success));
        return (success == RL_TRUE);
    }

private:

    ///
    /// Compile the shader for use. Returns true on a successful compile.
    ///
    inline bool compile() const
    {
        RLFunc(rlCompileShader(m_shader));
        return valid();
    }

    ///
    /// Create the shader.
    ///
    /// @param type Type of the shader to compile.
    ///
    /// @return If true, the shader was successfully created.
    ///
    inline bool createShader(const RLenum type)
    {
        assert(type == RL_VERTEX_SHADER ||
               type == RL_FRAME_SHADER ||
               type == RL_RAY_SHADER ||
               type == RL_PREFIX_RAY_SHADER);
        m_shader = RLFunc(rlCreateShader(type));
        return true;
    }

    RLshader m_shader = RL_NULL_SHADER; ///< OpenRL shader configured by this class.
};

} // namespace openrl

