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
#include <string_view>

namespace util {

struct LoadedTexture {
	std::shared_ptr<uint8_t> pixels = nullptr;
	openrl::Texture::Sampler sampler;
	openrl::Texture::Descriptor desc;
};

//-------------------------------------------------------------------------
// Load a texture off disk. Can specify both relative and absolute paths.
std::shared_ptr<openrl::Texture> loadTexture(const std::string_view path, bool generateMips = true, bool convertToLinear = true);

//-------------------------------------------------------------------------
// Use async tasks to load the texture. The resulting future can be used
// to query when the texture has finished loading.
std::future<LoadedTexture> loadTextureAsync(const std::string_view path, bool generateMips, bool convertToLinear);

//-------------------------------------------------------------------------
// Load LDR pixel data on the CPU. Data must be manually deallocated!
uint8_t* loadLDRTexturePixels(const std::string_view path, int& width, int& height, int& channelCount);

} // namespace util.
