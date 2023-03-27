//
//  RayGenerationConstants.h
//  Heatray5
//
//  Created by Cody White on 3/26/23.
//

#pragma once

#include <simd/simd.h>

struct RayGenerationConstants {
    simd::float4x4 viewMatrix;
    float fovTan = 0.0f;
    float aspectRatio = 1.0f;
    uint32_t renderWidth = 0;
    uint32_t renderHeight = 0;
};
