//
//  Shader.h
//  Heatray
//
//  C++ wrapper for an OpenRL shader.
//
//

#pragma once

#include "Error.h"

#include "../Utility/Log.h"

#include <OpenRL/rl.h>
#include <assert.h>
#include <string>
#include <vector>

namespace openrl {

//
// Class to contain an OpenRL shader. Handles shader loading and compilation.
//
class Shader
{
public:
    //-------------------------------------------------------------------------
    // Possible shader types.
    enum class ShaderType : uint8_t
    {
        kVertex,
        kFrame,
        kRay,
        kPrefix,

        kCount
    };

    ~Shader() {
        if (m_shader != RL_NULL_SHADER) {
            RLFunc(rlDeleteShader(m_shader));
            m_shader = RL_NULL_SHADER;
        }
    }
    Shader(const Shader& other) = default;
    Shader& operator=(const Shader& other) = default;

    //-------------------------------------------------------------------------
    // Create and compile a shader from a passed-in source string.
    static std::shared_ptr<Shader> createFromString(const std::string& shaderSource, const ShaderType type, const char* name)
    {
        if (shaderSource.length()) {
            Shader* shader = new Shader(type);
            RLFunc(rlShaderString(shader->shader(), RL_SHADER_NAME, name));

            const char* s = shaderSource.c_str();
            RLFunc(rlShaderSource(shader->shader(), 1, &s, nullptr));

            if (shader->compile()) {
                return std::shared_ptr<Shader>(shader);
            } else {
                const char* log = nullptr;
                RLFunc(rlGetShaderString(shader->shader(), RL_COMPILE_LOG, &log));
                LOG_ERROR("Unable to compile shader %s \n\t%s", name, log);
                return nullptr;
            }
        } else {
            LOG_ERROR("Attempting to build shader \"%s\" with an empty source file!", name);
        }

        return nullptr;
    }

    //-------------------------------------------------------------------------
    // Create and compile s ahder from multiple shader source strings.
    static std::shared_ptr<Shader> createFromMultipleStrings(const std::vector<std::string>& shaderSource, const ShaderType type, const char* name)
    {
        constexpr static size_t MAX_NUM_SHADER_STRINGS = 20;
        assert(shaderSource.size() < MAX_NUM_SHADER_STRINGS);

        if (shaderSource.size()) {
            Shader* shader = new Shader(type);
            RLFunc(rlShaderString(shader->shader(), RL_SHADER_NAME, name));

            const char* strings[MAX_NUM_SHADER_STRINGS];
            for (int iIndex = 0; iIndex < shaderSource.size(); ++iIndex) {
                assert(shaderSource[iIndex].length());
                strings[iIndex] = shaderSource[iIndex].c_str();
            }

            RLFunc(rlShaderSource(shader->shader(), shaderSource.size(), &strings[0], nullptr));

            if (shader->compile()) {
                return std::shared_ptr<Shader>(shader);
            } else {
                const char* log = nullptr;
                RLFunc(rlGetShaderString(shader->shader(), RL_COMPILE_LOG, &log));
                LOG_ERROR("Unable to compile shader %s \n\t%s", name, log);
                return nullptr;
            }
        } else {
            LOG_ERROR("Attempting to build shader \"%s\" with an empty source file!", name);
        }

        return nullptr;
    }

    //-------------------------------------------------------------------------
    // Get access to the internal OpenRL shader object.
    inline RLshader shader() const { return m_shader; }

    //-------------------------------------------------------------------------
    // Determine if this shader is considered valid and ready to be used by
    // OpenRL.
    inline bool valid() const
    {
        RLint success = 0;
        RLFunc(rlGetShaderiv(m_shader, RL_COMPILE_STATUS, &success));
        return (success == RL_TRUE);
    }

    inline ShaderType type() const { return m_type; }

private:
    explicit Shader(const ShaderType type) 
        : m_type(type) {
        RLenum rlType = 0;
        switch (type) {
            case ShaderType::kVertex:
                rlType = RL_VERTEX_SHADER;
                break;
            case ShaderType::kFrame:
                rlType = RL_FRAME_SHADER;
                break;
            case ShaderType::kRay:
                rlType = RL_RAY_SHADER;
                break;
            case ShaderType::kPrefix:
                rlType = RL_PREFIX_RAY_SHADER;
                break;
        };
        m_shader = RLFunc(rlCreateShader(rlType));
    }

    //-------------------------------------------------------------------------
    // Compile the shader for use. Returns true if the shader was successfully
    // compiled.
    inline bool compile() const
    {
        RLFunc(rlCompileShader(m_shader));
        return valid();
    }

    RLshader m_shader = RL_NULL_SHADER; // OpenRL shader configured by this class.

    const ShaderType m_type;
};

} // namespace openrl

