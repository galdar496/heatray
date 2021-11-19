//
//  TextureLoader.h
//  Heatray
//
//  Load various texture files off disk.
//
//

#pragma once

#include <RLWrapper/Texture.h>

#include <future>

namespace util {

struct LoadedTexture {
	std::shared_ptr<uint8_t> pixels = nullptr;
	openrl::Texture::Sampler sampler;
	openrl::Texture::Descriptor desc;
};

/// Load a texture off disk.
/// @param path Relative path or absolute path to the texture.
/// @return A valid texture setup with proper formats based on the image in "path".
std::shared_ptr<openrl::Texture> loadTexture(const char* path, bool generateMips = true, bool convertToLinear = true);

std::future<LoadedTexture> loadTextureAsync(const char* path, bool generateMips, bool convertToLinear);

uint8_t* loadLDRTexturePixels(const char* path, int& width, int& height, int& channelCount);

} // namespace util.
