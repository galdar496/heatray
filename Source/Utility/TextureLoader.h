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
openrl::Texture loadTexture(const char* path, bool generateMips = true, bool convertToLinear = true);

} // namespace util.
