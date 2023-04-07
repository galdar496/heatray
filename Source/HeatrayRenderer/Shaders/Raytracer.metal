//
//  Raytracer.metal
//  Heatray5 Shared
//
//  Created by Cody White on 3/17/23.
//

#import "Constants.h"
#import "Ray.h"
#import "RaytracerFrameConstants.h"

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

Ray createCameraRay(constant FrameCamera& frameCamera, const uint2 threadID) {
    float2 pixelUV = float2(threadID.x / frameCamera.renderWidth, threadID.y / frameCamera.renderHeight);
    
    // The following math assumes that the image plane is 1 unit away from the camera origin.
    float x = (2.0f * pixelUV.x - 1.0f) * frameCamera.aspectRatio * frameCamera.fovTan;
    float y = (1.0 - 2.0 * pixelUV.y) * frameCamera.fovTan;
    float3 dirCameraSpace = normalize(float3(x, y, -1.0f));
    
    Ray ray;
    ray.origin = (frameCamera.viewMatrix * float4(0.0f, 0.0f, 0.0f, 1.0f)).xyz;
    ray.direction = normalize((frameCamera.viewMatrix * float4(dirCameraSpace, 1.0f)).xyz);
    ray.weight = packed_float3(1.0f);
    ray.maxDistance = constants::INFINITE_MAXT;
    // TODO: determine the ray sequence ID and index.
    
    return ray;
}

// Responsible for generating pixel data for a single frame (sample index).
kernel void generateFrame(uint2 threadID [[thread_position_in_grid]],
                          constant RaytracerFrameConstants& constants,
                          texture2d<float, access::read> lastFrameTexture,
                          texture2d<float, access::write> destinationTexture) {
    constant FrameCamera& frameCamera = constants.camera;
    if (threadID.x < frameCamera.renderWidth && threadID.y < frameCamera.renderHeight) {
        Ray ray = createCameraRay(frameCamera, threadID);
        
        float4 lastFrameColor = (!constants.shouldClear) ? lastFrameTexture.read(threadID) : float4(0.0f);
        
        destinationTexture.write(float4(1.0f, 1.0f, 0.0f, 1.0f), threadID);
    }
}



