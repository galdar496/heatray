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

    //-------------------------------------------------------------------------
    // Create the PBO's internal dat.
    inline void create(RLint sizeInBytes)
    {
        m_buffer = openrl::Buffer::create(RL_PIXEL_PACK_BUFFER, nullptr, sizeInBytes, "Pixel data");
        m_sizeInBytes = sizeInBytes;
    }

    //-------------------------------------------------------------------------
    // Deallocate the internal OpenRL data for this PBO.
    inline void destroy() 
    {
        assert(!m_isMapped);
        m_buffer.reset(); 
    }

    //-------------------------------------------------------------------------
    // Supply the data source for this PBO.
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

    //-------------------------------------------------------------------------
    // Get a CPU pointer to the pixel data.
    inline const float* mapPixelData() const
    {
        assert(!m_isMapped);
        m_isMapped = true;
        m_buffer->bind();
        const float* imagePixels = m_buffer->mapBuffer<const float>(RL_READ_ONLY);
        return imagePixels;
    }

    //-------------------------------------------------------------------------
    // Invalidate the CPU pointer to the pixel data.
    inline void unmapPixelData() const
    {
        assert(m_isMapped);
        m_isMapped = false;
        m_buffer->unmapBuffer();
        m_buffer->unbind();
    }

    inline RLint size() const { return m_sizeInBytes; }
    inline RLint width() const { return m_width; }
    inline RLint height() const { return m_height; }

    inline bool mapped() const { return m_isMapped; }

    static constexpr RLint kNumChannels = 4; ///< Currently only supports 4 channels of data (RGBA).

private:

    std::shared_ptr<Buffer> m_buffer = nullptr;
    RLint m_sizeInBytes = -1;  // Size of the buffer.
    RLint m_width = -1; // Width of the buffer (in pixels).
    RLint m_height = -1; // Height of the buffer (in pixels).
    mutable bool m_isMapped = false;
};

} // namespace openrl.
