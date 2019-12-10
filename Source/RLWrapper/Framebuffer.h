//
//  Framebuffer.h
//  Heatray
//
//  
//
//

#pragma once

#include "Error.h"
#include "Texture.h"

#include <OpenRL/rl.h>
#include <assert.h>
#include <string>

namespace openrl
{

class Framebuffer
{
public:

    Framebuffer() = default;
    Framebuffer(const Framebuffer& other) = default;
    Framebuffer& operator=(const Framebuffer& other) = default;
    ~Framebuffer() = default;

    inline void create()
    {
        if (m_fbo == RL_NULL_FRAMEBUFFER)
        {
            RLFunc(rlGenFramebuffers(1, &m_fbo));
        }
    }

    inline void destroy()
    {
        if (m_fbo != RL_NULL_FRAMEBUFFER)
        {
            RLFunc(rlDeleteFramebuffers(1, &m_fbo));
            m_fbo = RL_NULL_FRAMEBUFFER;
        }
    }

    inline void addAttachment(const Texture& attachment, RLenum location) const
    {
        assert(attachment.valid());
        assert(m_fbo != RL_NULL_FRAMEBUFFER);

        bind();
        RLFunc(rlFramebufferTexture2D(RL_FRAMEBUFFER, location, RL_TEXTURE_2D, attachment.texture(), 0));
        unbind();
    }

    inline void bind() const
    {
        assert(m_fbo != RL_NULL_FRAMEBUFFER);
        RLFunc(rlBindFramebuffer(RL_FRAMEBUFFER, m_fbo));
    }

    inline void unbind() const
    {
        RLFunc(rlBindFramebuffer(RL_FRAMEBUFFER, RL_NULL_FRAMEBUFFER));
    }

    inline bool valid() const
    {
        bind();
        RLFunc(RLenum status = rlCheckFramebufferStatus(RL_FRAMEBUFFER));
        unbind();

        return (status == RL_FRAMEBUFFER_COMPLETE);
    }

private:

    RLframebuffer m_fbo = RL_NULL_FRAMEBUFFER;
};

} // namespace openrl.