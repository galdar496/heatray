//
//  RayShader.metal
//  Heatray5 macOS
//
//  Created by Cody White on 4/15/23.
//

#import "FrameConstants.h"
#import "Ray.h"

#include <metal_stdlib>
using namespace metal;

struct Intersection {
    // The distance from the ray origin to the intersection point. Negative if the ray did not
    // intersect the scene.
    float distance;
    
    // The index of the intersected primitive (triangle), if any. Undefined if the ray did not
    // intersect the scene.
    int primitiveIndex;
    
    // The barycentric coordinates of the intersection point, if any. Undefined if the ray did
    // not intersect the scene.
    float2 barycentrics;
};

template<typename T>
inline T interpolateVertexAttribute(device T* attributeBuffer, Intersection intersection) {
    float3 barycentrics;
    barycentrics.xy = intersection.barycentrics;
    barycentrics.z = 1.0f - barycentrics.x - barycentrics.y;
    
    uint32_t triangleIndex = intersection.primitiveIndex;
    
    // Get the value for each vertex.
    T x = attributeBuffer[triangleIndex * 3 + 0];
    T y = attributeBuffer[triangleIndex * 3 + 1];
    T z = attributeBuffer[triangleIndex * 3 + 2];
    
    // Return the interpolated value.
    return (barycentrics.x * x) + (barycentrics.y * y) + (barycentrics.z * z);
}

kernel void shadeRays(uint2 threadID [[thread_position_in_grid]],
                      constant FrameGlobals& frameGlobals,
                      device Ray* rays,
                      device Intersection* intersections,
                      device float3* vertexNormals,
                      texture2d<float, access::write> dstTexture /* THIS IS TEMPORARY */) {
    if (threadID.x < frameGlobals.frameWidth && threadID.y < frameGlobals.frameWidth) {
        uint32_t rayIndex = threadID.y * frameGlobals.frameWidth + threadID.x;
        device Ray& ray = rays[rayIndex];
        device Intersection& intersection = intersections[rayIndex];
        
        if (ray.maxDistance >= 0.0f && intersection.distance >= 0.0f) {
            //float3 intersectionPoint = ray.origin + ray.direction * intersection.distance;
            
            float3 surfaceNormal = interpolateVertexAttribute(vertexNormals, intersection);
            surfaceNormal = normalize(surfaceNormal);
            
            // TEMPORARY!!
            dstTexture.write(float4((surfaceNormal + 1.0f) * 0.5f, 1.0f), threadID);
        }
    }
}
