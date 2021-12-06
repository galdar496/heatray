//
//  Material.h
//  Heatray
//
//  Base class for all materials represented in Heatray.
//

#pragma once

#include <RLWrapper/Buffer.h>
#include <RLWrapper/Program.h>

#include <memory>

class Material
{
public:
    // Supported material types.
    enum class Type {
        PBR,
        Glass
    };

    explicit Material(const std::string& name, Type type)
        : m_name(name)
        , m_type(type) {}
    virtual ~Material() = default;

    std::shared_ptr<openrl::Program> program() { return m_program; }
    std::shared_ptr<openrl::Buffer> uniformBlock() { return m_constants; }
    const std::string& name() const { return m_name; }
    Type type() const { return m_type; }

    //-------------------------------------------------------------------------
    // Builds the OpenRL program along with any buffers etc that need to be
    // populated.
    virtual void build() = 0;

    //-------------------------------------------------------------------------
    // Completely destroys all internal data and rebuilds it from scratch, 
    // including reloading any shader data.
    virtual void rebuild() = 0;

    //-------------------------------------------------------------------------
    // Upload parameter changes to OpenRL.
    virtual void modify() = 0;
protected:
    static constexpr char const * m_vertexShader = "vertex.rlsl";

    std::shared_ptr<openrl::Buffer>  m_constants = nullptr; // Constants used by this material. Will be uploaded as a uniform block to the corresponding shader.
    std::shared_ptr<openrl::Program> m_program   = nullptr; // Shader representing this material.

    const std::string m_name;
    Type m_type;
};
