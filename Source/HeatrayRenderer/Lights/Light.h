//
//  Light.h
//  Heatray
//
//  Base class for all lights.
//

#pragma once

#include "ShaderLightingDefines.h"

#include <RLWrapper/Primitive.h>
#include <RLWrapper/Program.h>

#include <string>
#include <string_view>

class Light
{
public:
    // Supported light types.
    enum class Type {
        kEnvironment,
        kDirectional,
        kPoint,
        kSpot
    };

    explicit Light(const std::string_view name, const Type type)
        : m_name(name)
        , m_type(type) {}
    virtual ~Light() = default;

    std::shared_ptr<openrl::Program> program() const { return m_program; }
    std::shared_ptr<openrl::Primitive> primitive() const { return m_primitive; }
    std::string_view name() const { return m_name; }
    Type type() const { return m_type; }
protected:
    std::shared_ptr<openrl::Primitive> m_primitive = nullptr;
    std::shared_ptr<openrl::Program> m_program = nullptr;

    const std::string m_name;
    const Type m_type;
};
