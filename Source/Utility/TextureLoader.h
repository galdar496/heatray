//
//  TextureLoader.h
//  Heatray
//
//  Load various texture files off disk.
//
//

#pragma once

#include <RLWrapper/Texture.h>

namespace util {

/// Load a texture off disk.
/// @param path Relative path or absolute path to the texture.
/// @return A valid texture setup with proper formats based on the image in "path".
std::shared_ptr<openrl::Texture> loadTexture(const char* path, bool generateMips = true, bool convertToLinear = true);

uint8_t* loadLDRTexturePixels(const char* path, int& width, int& height, int& channelCount);

} // namespace util.
