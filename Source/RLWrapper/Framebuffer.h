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

    //-------------------------------------------------------------------------
    // Create an OpenRL FBO and return a shared_ptr to it.
    static std::shared_ptr<Framebuffer> create() {
        return std::shared_ptr<Framebuffer>(new Framebuffer);
    }

    //-------------------------------------------------------------------------
    // Attach a valid texture to this FBO.
    inline void addAttachment(std::shared_ptr<Texture> attachment, RLenum location)
    {
        assert(attachment->valid());
        assert(m_fbo != RL_NULL_FRAMEBUFFER);

        bind();
        RLFunc(rlFramebufferTexture2D(RL_FRAMEBUFFER, location, RL_TEXTURE_2D, attachment->texture(), 0));
        unbind();

        m_attachment = attachment;
    }

    //-------------------------------------------------------------------------
    // Bind this FBO for use by OpenRL.
    inline void bind() const
    {
        assert(m_fbo != RL_NULL_FRAMEBUFFER);
        RLFunc(rlBindFramebuffer(RL_FRAMEBUFFER, m_fbo));
    }

    //-------------------------------------------------------------------------
    // Tell OpenRL that this FBO should no longer be used.
    inline void unbind() const
    {
        RLFunc(rlBindFramebuffer(RL_FRAMEBUFFER, RL_NULL_FRAMEBUFFER));
    }

    //-------------------------------------------------------------------------
    // Validate that this FBO is valid and can be used by OpenRL.
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