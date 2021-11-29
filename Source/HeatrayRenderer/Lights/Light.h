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

class Light
{
public:
    Light() = default;
    virtual ~Light() = default;

    std::shared_ptr<openrl::Program> program() const { return m_program; }
    std::shared_ptr<openrl::Primitive> primitive() const { return m_primitive; }
protected:
    std::shared_ptr<openrl::Primitive> m_primitive = nullptr;
    std::shared_ptr<openrl::Program> m_program = nullptr;
};
