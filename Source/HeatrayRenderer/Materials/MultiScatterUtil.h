//
//  MultiScatterUtil.h
//  Heatray
//
//  Utility for generating a LUT to perform multi-scattering
//  for the PBR specular BRDF.
//
//

#pragma once

#include <RLWrapper/Texture.h>

#include <string>

//-------------------------------------------------------------------------
// Generates the multiscatter texture to use within shaders. This process
// can take quite some time so should be done offline.
void generateMultiScatterTexture();

//-------------------------------------------------------------------------
// Load the multiscatter texture. Repeated calls to this function will get
// the same shared pointer instead of reloading the multiscatter texture
// continuously.
std::shared_ptr<openrl::Texture> loadMultiscatterTexture();
