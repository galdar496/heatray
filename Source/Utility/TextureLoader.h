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

//-------------------------------------------------------------------------
// Load a texture off disk. Can specify both relative and absolute paths.
std::shared_ptr<openrl::Texture> loadTexture(const char* path, bool generateMips = true, bool convertToLinear = true);

//-------------------------------------------------------------------------
// Use async tasks to load the texture. The resulting future can be used
// to query when the texture has finished loading.
std::future<LoadedTexture> loadTextureAsync(const char* path, bool generateMips, bool convertToLinear);

//-------------------------------------------------------------------------
// Load LDR pixel data on the CPU. Data must be manually deallocated!
uint8_t* loadLDRTexturePixels(const char* path, int& width, int& height, int& channelCount);

} // namespace util.
