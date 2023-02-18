#include "TextureLoader.h"

#include "Log.h"

#include <FreeImage/FreeImage.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include <assert.h>
#include <cmath>
#include <filesystem>
#include <fstream>

namespace util {

void loadTextureInternal(LoadedTexture& loadedTexture, const std::string_view path, bool generateMips, bool convertToLinear)
{
    LOG_INFO("Loading texture %s", path);
    
    // Make sure the file exists. It may be one directory back as well.
    std::string finalPath = std::string(path);
    {
        std::ifstream fin;
        fin.open(finalPath);
        if (!fin) {
            finalPath = "../" + finalPath;
            fin.open(finalPath);
            if (!fin) {
                LOG_ERROR("Unable to find texture %s", path);
                return;
            }
        }
        fin.close();
    }
    
    if (!generateMips) { // default sampler state is with mipmapping enabled.
        loadedTexture.sampler.magFilter = RL_LINEAR;
        loadedTexture.sampler.minFilter = RL_LINEAR;
        loadedTexture.sampler.wrapS = RL_CLAMP_TO_EDGE;
        loadedTexture.sampler.wrapT = RL_CLAMP_TO_EDGE;
    }

    if ((std::filesystem::path(finalPath).extension() == ".exr") ||
        (std::filesystem::path(finalPath).extension() == ".tiff")) {
        FREE_IMAGE_FORMAT format = FreeImage_GetFileType(finalPath.c_str(), 0);
        assert(FreeImage_FIFSupportsReading(format));

        // Get the raw image data from FreeImage.
        FIBITMAP* imageData = FreeImage_Load(format, finalPath.c_str());
        if (!imageData) {
            LOG_ERROR("Unable to load image %s", path);
            return;
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

        loadedTexture.desc.dataType = textureDataType;
        loadedTexture.desc.format = textureFormat;
        loadedTexture.desc.internalFormat = textureFormat;
        loadedTexture.desc.width = FreeImage_GetWidth(imageData);
        loadedTexture.desc.height = FreeImage_GetHeight(imageData);

        loadedTexture.pixels = std::shared_ptr<uint8_t>(FreeImage_GetBits(imageData), [imageData](void *address) {
            FreeImage_Unload(imageData);
        });
    } else {
        int width, height, channelCount;
        stbi_set_flip_vertically_on_load(true);

        unsigned char* pixels;
        bool isHDR = stbi_is_hdr(finalPath.c_str());
        if (isHDR) {
            pixels = (unsigned char*)stbi_loadf(finalPath.c_str(), &width, &height, &channelCount, 0);
            if (!pixels) {
                LOG_ERROR("Unable to load texture %s", path);
                return;
            }
        } else {
            pixels = stbi_load(finalPath.c_str(), &width, &height, &channelCount, 0);
            if (!pixels) {
                LOG_ERROR("Unable to load texture %s", path);
                return;
            }

            if (convertToLinear) {
                LOG_INFO("Converting from sRGB to Linear");
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
                            } else {
                                channelData = std::powf((channelData + SRGB_ALPHA) / (1.0f + SRGB_ALPHA), 2.4f);
                            }

                            // Conversion back to an 8bit byte.
                            pixels[pixelIndex] = uint8_t(channelData * MAX_BYTE_VALUE);
                        }
                    }
                }
                LOG_INFO("\tDONE");
            }
        }

        loadedTexture.desc.width = width;
        loadedTexture.desc.height = height;
        switch (channelCount) {
            case 1:
                loadedTexture.desc.format = RL_LUMINANCE; break;
            case 3:
                loadedTexture.desc.format = RL_RGB; break;
            case 4:
                loadedTexture.desc.format = RL_RGBA; break;
            default:
            {
                LOG_WARNING("Texture \"%s\" has an unsupported channel count: %d -- skipping.", path, channelCount);
                loadedTexture.pixels = nullptr;
                return;
            }
        }
        loadedTexture.desc.internalFormat = loadedTexture.desc.format;
        loadedTexture.desc.dataType = isHDR ? RL_FLOAT : RL_UNSIGNED_BYTE;

        loadedTexture.pixels = std::shared_ptr<uint8_t>(pixels, [](void *address) {
            free(address);
        });
    }
}

std::future<LoadedTexture> loadTextureAsync(const std::string_view path, bool generateMips, bool convertToLinear)
{
    std::shared_ptr<uint8_t> pixels = nullptr;
    std::string filepath = std::string(path);
    return std::async([pixels, filepath, generateMips, convertToLinear]() {
        LoadedTexture loadedTexture;
        loadTextureInternal(loadedTexture, filepath.c_str(), generateMips, convertToLinear);
        return loadedTexture;
    });
}

std::shared_ptr<openrl::Texture> loadTexture(const std::string_view path, bool generateMips, bool convertToLinear)
{
    LoadedTexture loadedTexture;
    loadTextureInternal(loadedTexture, path, generateMips, convertToLinear);
    std::shared_ptr<openrl::Texture> texture = nullptr;

    if (loadedTexture.pixels) {
        texture = openrl::Texture::create(loadedTexture.pixels.get(), loadedTexture.desc, loadedTexture.sampler, generateMips);
        assert(texture->valid());
    }

    return texture;
}

uint8_t* loadLDRTexturePixels(const std::string_view path, int& width, int& height, int& channelCount)
{
    stbi_uc* pixels = stbi_load(path.data(), &width, &height, &channelCount, 0);
    return static_cast<uint8_t*>(pixels);
}

} // namespace util.
