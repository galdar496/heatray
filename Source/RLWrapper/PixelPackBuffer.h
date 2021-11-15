//
//  PixelPackBuffer.h
//  Heatray
//
//  Setup a simple pixel pack buffer for manipulation of pixel data.
//
//

#pragma once

#include "Buffer.h"
#include "Error.h"
#include "Texture.h"

#include <assert.h>
#include <memory>

namespace openrl {

class PixelPackBuffer
{
public:
    PixelPackBuffer() = default;
    PixelPackBuffer(const PixelPackBuffer& other) = default;
    PixelPackBuffer& operator=(const PixelPackBuffer& other) = default;
    ~PixelPackBuffer() = default;

    /// @param size Size to create the buffer (in bytes).
    inline void create(RLint size)
    {
        m_buffer = openrl::Buffer::create(RL_PIXEL_PACK_BUFFER, nullptr, size, "Pixel data");
        m_size = size;
    }

    inline void destroy() 
    {
        assert(!m_isMapped);
        m_buffer.reset(); 
    }

    inline void setPixelData(const Texture& texture)
    {
        assert(!m_isMapped);
        m_buffer->bind();
        RLFunc(rlBindTexture(RL_TEXTURE_2D, texture.texture()));
        RLFunc(rlGetTexImage(RL_TEXTURE_2D, 0, RL_RGBA, RL_FLOAT, 0)); // TODO: make this more general.
        m_buffer->unbind();

        m_width  = texture.width();
        m_height = texture.height();
    }

    inline const float* mapPixelData() const
    {
        assert(!m_isMapped);
        m_isMapped = true;
        m_buffer->bind();
        const float* imagePixels = m_buffer->mapBuffer<const float>(RL_READ_ONLY);
        return imagePixels;
    }

    inline void unmapPixelData() const
    {
        assert(m_isMapped);
        m_isMapped = false;
        m_buffer->unmapBuffer();
        m_buffer->unbind();
    }

    inline RLint size() const { return m_size; }
    inline RLint width() const { return m_width; }
    inline RLint height() const { return m_height; }

    inline bool mapped() const { return m_isMapped; }

    static constexpr RLint kNumChannels = 4; ///< Currently only supports 4 channels of data (RGBA).

private:

    std::shared_ptr<Buffer> m_buffer = nullptr;
    RLint m_size = -1;   ///< Size of the buffe (in bytes).
    RLint m_width = -1;  ///< Width of the buffer (in pixels).
    RLint m_height = -1; ///< Height of the buffer (in pixels).
    mutable bool m_isMapped = false;
};

} // namespace openrl.
