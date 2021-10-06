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

void generateMultiScatterTexture();

std::shared_ptr<openrl::Texture> loadMultiscatterTexture();
