//
//  OrbitCamera.h
//  Heatray
//
//  
//

#pragma once

#include "UIEditableSIMD.h"

#include <simd/simd.h>
#include <algorithm>
#include <assert.h>

struct OrbitCamera
{
    float distance = 19.0f;                   // Distance to the camera from the look-at position in world-space (in meters).
    float phi      = 0.0f;                    // In radians [0 - 2π]
    float theta    = 0.0f;                    // In radians [-π/2 - π/2]
    UIEditableSIMD<simd::float3> target;      // Position to look at in world-space.

    float max_distance = 100.0f; // Maximum distance the camera can go.

    //-------------------------------------------------------------------------
    // Generate a right-handed Y-up view matrix given the current parameters
    // of the orbital camera.
    simd::float4x4 createViewMatrix() const
    {
        const simd::float3 right = simd::make_float3(1.0f, 0.0f, 0.0f);
        const simd::float3 up    = simd::make_float3(0.0f, 1.0f, 0.0f);

        simd::quatf rotTheta = simd::quatf(theta, right);
        simd::quatf rotPhi = simd::quatf(phi, up);
        simd::quatf orientation = rotTheta * rotPhi;
        
        simd::float4x4 translation = matrix_identity_float4x4;
        translation.columns[3] = simd::make_float4(target.simdValue(), 0.0f) + simd::make_float4(0.0f, 0.0f, distance, 1.0f);
        
        simd::float4x4 viewMatrix = simd::float4x4(simd::inverse(orientation)) * translation;
        return viewMatrix;
    }
};
