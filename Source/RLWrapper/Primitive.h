//
//  Primitive.h
//  Heatray
//
//  
//
//

#pragma once

#include "Error.h"
#include "Program.h"

#include <OpenRL/OpenRL.h>

#include <memory>

namespace openrl {

class Primitive
{
public:
    ~Primitive() {
        if (m_primitive != RL_NULL_PRIMITIVE) {
            RLFunc(rlDeletePrimitives(1, &m_primitive));
        }
    }

    Primitive(const Primitive& other) = default;
    Primitive& operator=(const Primitive& other) = default;

    //-------------------------------------------------------------------------
    // Create the internal OpenRL objects for a primitive and return a shared_ptr.
    static std::shared_ptr<Primitive> create() {
        return std::shared_ptr<Primitive>(new Primitive);
    }

    //-------------------------------------------------------------------------
    // Attach a valid program to this primitive.
    inline void attachProgram(std::shared_ptr<Program> program)
    {
        assert(program->valid());
        bind();
        program->bind();
        unbind();

        m_attachedProgram = program;
    }

    //-------------------------------------------------------------------------
    // Various getters and setters for a primitive.
    inline void bind() const { RLFunc(rlBindPrimitive(RL_PRIMITIVE, m_primitive)); }
    inline void unbind() const { RLFunc(rlBindPrimitive(RL_PRIMITIVE, RL_NULL_PRIMITIVE)); }
    inline const RLprimitive primitive() const { return m_primitive; }
    inline bool valid() const { return m_primitive != RL_NULL_PRIMITIVE; }

private:
    Primitive() {
        RLFunc(rlGenPrimitives(1, &m_primitive));
    }

    RLprimitive m_primitive = RL_NULL_PRIMITIVE;

    std::shared_ptr<Program> m_attachedProgram = nullptr;
};

} // namespace openrl.