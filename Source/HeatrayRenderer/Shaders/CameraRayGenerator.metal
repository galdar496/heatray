//
//  CameraRayGenerator.metal
//  Heatray5 Shared
//
//  Created by Cody White on 3/17/23.
//

#import "FrameConstants.h"
#import "Ray.h"
#import "ShaderMathConstants.h"

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

// Responsible for generating pixel data for a single frame (sample index).
kernel void generateCameraRays(uint2 threadID [[thread_position_in_grid]],
                               constant FrameGlobals& frameGlobals,
                               constant CameraRayGeneratorConstants& constants,
                               device Ray* generatedRays,
                               texture2d<float, access::write> accumulationTexture) {
    if (threadID.x < frameGlobals.frameWidth && threadID.y < frameGlobals.frameHeight) {
        // Determine this ray's position in the ray buffer.
        uint32_t rayIndex = threadID.y * frameGlobals.frameWidth + threadID.x;
        
        device Ray& ray = generatedRays[rayIndex];
        
        // Determine the proper origin and direction for this ray.
        {
            constant FrameCamera& frameCamera = constants.camera;
            float2 pixelUV = float2(float(threadID.x) / float(frameGlobals.frameWidth), float(threadID.y) / float(frameGlobals.frameHeight));
            
            // The following math assumes that the image plane is 1 unit away from the camera origin.
            float x = (2.0f * pixelUV.x - 1.0f) * frameCamera.aspectRatio * frameCamera.fovTan;
            float y = (1.0 - 2.0 * pixelUV.y) * frameCamera.fovTan;
            float3 dirCameraSpace = normalize(float3(x, y, -1.0f));
            
            ray.origin = (frameCamera.viewMatrix * float4(0.0f, 0.0f, 0.0f, 1.0f)).xyz;
            ray.direction = normalize((frameCamera.viewMatrix * float4(dirCameraSpace, 1.0f)).xyz);
            ray.weight = packed_float3(1.0f);
            ray.maxDistance = sm_constants::INFINITE_MAXT;
            // TODO: determine the ray sequence ID and index.
        }
        
        // If we've been asked to clear the accumulate texture, do so now.
        if (constants.shouldClear) {
            accumulationTexture.write(float4(0.0f), threadID);
        }
    }
}



