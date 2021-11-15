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
#include <memory>
#include <string>

namespace openrl {

class Framebuffer
{
public:
    ~Framebuffer() {
        if (m_fbo != RL_NULL_FRAMEBUFFER) {
            RLFunc(rlDeleteFramebuffers(1, &m_fbo));
            m_fbo = RL_NULL_FRAMEBUFFER;
        }
    }
    Framebuffer(const Framebuffer& other) = default;
    Framebuffer& operator=(const Framebuffer& other) = default;

    static std::shared_ptr<Framebuffer> create() {
        return std::shared_ptr<Framebuffer>(new Framebuffer);
    }

    inline void addAttachment(std::shared_ptr<Texture> attachment, RLenum location)
    {
        assert(attachment->valid());
        assert(m_fbo != RL_NULL_FRAMEBUFFER);

        bind();
        RLFunc(rlFramebufferTexture2D(RL_FRAMEBUFFER, location, RL_TEXTURE_2D, attachment->texture(), 0));
        unbind();

        m_attachment = attachment;
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
    Framebuffer() {
        RLFunc(rlGenFramebuffers(1, &m_fbo));
    }

    RLframebuffer m_fbo = RL_NULL_FRAMEBUFFER;
    std::shared_ptr<Texture> m_attachment = nullptr;
};

} // namespace openrl.