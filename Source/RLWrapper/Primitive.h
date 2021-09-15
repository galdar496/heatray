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

namespace openrl {

class Primitive
{
public:
    Primitive() = default;
    Primitive(const Primitive& other) = default;
    Primitive& operator=(const Primitive& other) = default;
    ~Primitive() = default;

    inline void create() 
    {
        if (m_primitive == RL_NULL_PRIMITIVE) {
            RLFunc(rlGenPrimitives(1, &m_primitive));
        }
    }

    inline void destroy()
    {
        if (m_primitive != RL_NULL_PRIMITIVE) {
            RLFunc(rlDeletePrimitives(1, &m_primitive));
        }
    }

    inline void attachProgram(const Program& program)
    {
        assert(program.valid());
        bind();
        program.bind();
        unbind();
    }

    inline void bind() const { RLFunc(rlBindPrimitive(RL_PRIMITIVE, m_primitive)); }
    inline void unbind() const { RLFunc(rlBindPrimitive(RL_PRIMITIVE, RL_NULL_PRIMITIVE)); }
    inline const RLprimitive primitive() const { return m_primitive; }
    inline bool valid() const { return m_primitive != RL_NULL_PRIMITIVE; }

private:

    RLprimitive m_primitive = RL_NULL_PRIMITIVE;
};

} // namespace openrl.