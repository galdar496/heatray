//
//  Shaders.metal
//  Heatray5 Shared
//
//  Created by Cody White on 3/17/23.
//

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
        // Compute linear ray index from 2D position
        unsigned int rayIdx = thread_id.y * constants.renderWidth + thread_id.x;
        device Ray& ray = rays[rayIdx];
        
        // Fill out the ray data here.
    }
}



