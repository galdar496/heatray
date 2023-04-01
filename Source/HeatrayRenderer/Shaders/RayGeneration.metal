//
//  Shaders.metal
//  Heatray5 Shared
//
//  Created by Cody White on 3/17/23.
//

#import "Constants.h"
#import "Ray.h"
#import "RayGenerationConstants.h"

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

// Responsible for generation rays from a virtual camera.
kernel void rayGeneration(uint2 thread_id [[thread_position_in_grid]],
                          constant RayGenerationConstants& constants,
                          device Ray* rays) {
    if (thread_id.x < constants.renderWidth && thread_id.y < constants.renderHeight) {
        float2 pixelUV = float2(thread_id.x / constants.renderWidth, thread_id.y / constants.renderHeight);
        
        // The following math assumes that the image plane is 1 unit away from the camera origin.
        float x = (2.0f * pixelUV.x - 1.0f) * constants.aspectRatio * constants.fovTan;
        float y = (1.0 - 2.0 * pixelUV.y) * constants.fovTan;
        float3 dirCameraSpace = normalize(float3(x, y, -1.0f));
        
        // Compute linear ray index from 2D position
        unsigned int rayIdx = thread_id.y * constants.renderWidth + thread_id.x;
        
        device Ray& ray = rays[rayIdx];
        ray.origin = (constants.viewMatrix * float4(0.0f, 0.0f, 0.0f, 1.0f)).xyz;
        ray.direction = normalize((constants.viewMatrix * float4(dirCameraSpace, 1.0f)).xyz);
        ray.weight = packed_float3(1.0f);
        ray.maxDistance = constants::INFINITE_MAXT;
        // TODO: determine the ray sequence ID and index.
    }
}



