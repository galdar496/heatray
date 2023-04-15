//
//  CameraRayGeneratorConstants.h
//  Heatray5
//
//  Created by Cody White on 3/26/23.
//

#pragma once

#include <simd/simd.h>

struct FrameCamera {
    simd::float4x4 viewMatrix;
    float fovTan = 0.0f;
    float aspectRatio = 1.0f;
};

struct CameraRayGeneratorConstants {
    FrameCamera camera;
    bool shouldClear = false;
};

struct FrameGlobals {
    uint32_t frameWidth = 0;
    uint32_t frameHeight = 0;
    unsigned int sampleIndex = 0;
};
