//
//  Pixels.h
//  Heatray
//
//  Created by Cody White on 1/04/16.
//
//

#pragma once

#include "../gfx/Buffer.h"
#include "../gfx/Texture.h"
#include <assert.h>

///
/// Pixel object which is used to get the pixel info out of the renderer.
///

class Pixels
{
    public:

        Pixels() :
            m_width(0),
            m_height(0),
            m_isMapped(false)
        {
            m_buffer.SetTarget(RL_PIXEL_PACK_BUFFER);
        }

        ~Pixels()
        {
            Destroy();
        }

        ///
        /// Destroy the pixel object.
        ///
        inline void Destroy()
        {
            m_buffer.Destroy();
        }

        ///
        /// Resize the pixel object with a specific size (but with no data).
        ///
        /// @param width New width (in pixels).
        /// @param height New height (in pixels).
        ///
        inline void Resize(size_t width, size_t height)
        {
            assert(!m_isMapped);
            m_buffer.Destroy();

            // Allocate a pixel-pack buffer with enough memory to store a m_num_pixel_channels-channel
            // floating-point image.
            m_buffer.Load(nullptr, width * height * sizeof(float) * NUM_PIXEL_CHANNELS, "Rendered Pixels");

            m_width  = width;
            m_height = height;
        }

        ///
        /// Set the pixel data by copying it from a texture.
        ///
        /// @param texture Texture to copy the data from. This texture should be in floating-point format.
        ///
        inline void SetData(const gfx::Texture &texture)
        {
            assert(!m_isMapped);
            m_buffer.Bind();
            rlBindTexture(RL_TEXTURE_2D, texture.GetTexture());
            rlGetTexImage(RL_TEXTURE_2D, 0, RL_RGB, RL_FLOAT, nullptr);
            m_buffer.Unbind();
        }

        ///
        /// Map the pixel data to the CPU for easy retrieval. 
        /// NOTE: Ensure to call 'unmapPixelData()' when finished with the mapped data.
        ///
        /// @return Pointer to the mapped pixel data on the CPU.
        ///
        inline const float *MapPixelData()
        {
            assert(!m_isMapped);

            m_buffer.Bind();
            float *image_pixels = m_buffer.MapBuffer<float>(RL_READ_ONLY);
            m_isMapped = true;
            return image_pixels;
        }

        ///
        /// Unmap the pixel data.
        ///
        inline void UnmapPixelData()
        {
            assert(m_isMapped);

            m_buffer.UnmapBuffer();
            m_buffer.Unbind();
            m_isMapped = false;
        }

        ///
        /// Get the dimensions of the pixel buffer.
        ///
        /// @param width Populated width (in pixels).
        /// @param height Populated height (in pixels).
        ///
        inline void GetDimensions(size_t &width, size_t &height)
        {
            width  = m_width;
            height = m_height;
        }

        static const int NUM_PIXEL_CHANNELS = 3;

    private:

        gfx::Buffer m_buffer; ///< Pixel-pack buffer containing the pixel information.

        size_t m_width; ///< Width of the pixel buffer (in pixels).
        size_t m_height; ///< Height of the pixel buffer (in pixels).

        bool m_isMapped; ///< If true, the buffer is currently mapped to the CPU.
};
