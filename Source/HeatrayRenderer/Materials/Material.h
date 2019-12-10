//
//  Material.h
//  Heatray
//
//  Base class for all materials represented in Heatray.
//

#pragma once

#include <RLWrapper/Buffer.h>
#include <RLWrapper/Program.h>

class Material
{
public:
    Material() = default;
    virtual ~Material() = default;

    openrl::Program& program() { return m_program; }
    openrl::Buffer& uniformBlock() { return m_constants; }
protected:
    static constexpr char const * m_vertexShader = "positionNormal.vert.rlsl";

    openrl::Buffer m_constants; ///< Constants used by this material. Will be uploaded as a uniform block to the corresponding shader.
    openrl::Program m_program; ///< Shader representing this material.
};
