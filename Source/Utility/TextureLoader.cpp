#include "TextureLoader.h"

#include "Log.h"

#include <FreeImage/FreeImage.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include <assert.h>
#include <filesystem>

namespace util {

openrl::Texture loadTexture(const char* path, bool generateMips, bool convertToLinear)
{
    openrl::Texture::Sampler sampler;
    if (!generateMips) { // default sampler state is with mipmapping enabled.
        sampler.magFilter = RL_NEAREST;
        sampler.minFilter = RL_NEAREST_MIPMAP_NEAREST;
    }

    openrl::Texture texture;

    if (std::filesystem::path(path).extension() == ".exr") {
        FREE_IMAGE_FORMAT format = FreeImage_GetFileType(path, 0);
        assert(FreeImage_FIFSupportsReading(format));

        // Get the raw image data from FreeImage.
        FIBITMAP* imageData = FreeImage_Load(format, path);
        if (!imageData)
        {
			LOG_ERROR("Unable to load image %s", path);
            return openrl::Texture();
        }

        FREE_IMAGE_TYPE type = FreeImage_GetImageType(imageData);

        RLenum textureDataType = -1;
        RLenum textureFormat = -1;
        switch (type) {
            // Likely some more to do here for other image types.
            case FIT_RGBF:
                textureDataType = RL_FLOAT;
                textureFormat   = RL_RGB;
                break;
            case FIT_BITMAP:
                textureDataType = RL_UNSIGNED_BYTE;
                textureFormat   = RL_RGBA;
                break;
            case FIT_FLOAT:
                textureDataType = RL_FLOAT;
                textureFormat   = RL_LUMINANCE;
                break;
            default:
                assert(0 && "Unimplemented texture type");;
        }

        openrl::Texture::Descriptor desc;
        desc.dataType       = textureDataType;
        desc.format         = textureFormat;
        desc.internalFormat = textureFormat;
        desc.width          = FreeImage_GetWidth(imageData);
        desc.height         = FreeImage_GetHeight(imageData);

        texture.create(FreeImage_GetBits(imageData), desc, sampler, generateMips);
        FreeImage_Unload(imageData);
    } else {
        int width, height, channelCount;
        stbi_set_flip_vertically_on_load(true);

        unsigned char * pixels;
        bool isHDR = stbi_is_hdr(path);
        if (isHDR) {
            pixels = (unsigned char *)stbi_loadf(path, &width, &height, &channelCount, 0);
        } else {
            pixels = stbi_load(path, &width, &height, &channelCount, 0);

			if (convertToLinear) {
				// Convert from sRGB to linear. Note: it's assumed that any non-HDR immage is sRGB encoded however
				// we want linear colors for rendering.
				constexpr static size_t ALPHA_CHANNEL = 3;
				constexpr static float MAX_BYTE_VALUE = 255.0f;
				constexpr static float SRGB_ALPHA = 0.055f;
				for (size_t i = 0; i < width * height; ++i) {
					// For each pixel, we alter the RGB components but leave any alpha channel alone.
					for (size_t channel = 0; channel < channelCount; ++channel) {
						if (channel != ALPHA_CHANNEL) {
							size_t pixelIndex = (i * channelCount) + channel;
							float channelData = float(pixels[pixelIndex]) / MAX_BYTE_VALUE;

							// Actual sRGB->linear convertion.
							if (channelData <= 0.04045f) {
								channelData /= 12.92f;
							}
							else {
								channelData = std::powf((channelData + SRGB_ALPHA) / (1.0f + SRGB_ALPHA), 2.4f);
							}

							// Conversion back to an 8bit byte.
							pixels[pixelIndex] = uint8_t(channelData * MAX_BYTE_VALUE);
						}
					}
				}
			}
        }

        openrl::Texture::Descriptor desc;
        desc.width = width;
        desc.height = height;
        switch (channelCount) {
            case 1:
                desc.format = RL_FLOAT; break;
            case 3:
                desc.format = RL_RGB; break;
            case 4:
                desc.format = RL_RGBA; break;
            default:
                assert(0 && "Unsupported channel count");;
        }
        desc.internalFormat = desc.format;
        desc.dataType = isHDR ? RL_FLOAT : RL_UNSIGNED_BYTE;

        texture.create(pixels, desc, sampler, generateMips);
        free(pixels);
    }
    
    assert(texture.valid());

    // Copy elision should turn this into a move.
    return texture;
}

}