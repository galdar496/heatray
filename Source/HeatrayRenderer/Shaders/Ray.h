//
//  Ray.h
//  Heatray5
//
//  Created by Cody White on 3/26/23.
//

#pragma once

#include <simd/simd.h>

struct __attribute__((packed)) PackedFloat3 {
    float x;
    float y;
    float z;
};

#ifdef __METAL_VERSION__
    #define PACKED_FLOAT3 packed_float3
#else
    #define PACKED_FLOAT3 PackedFloat3
#endif

struct Ray {
    PACKED_FLOAT3 origin;      // Origin of the ray.
    int sequenceID;            // Which sequence should this ray be using.
    PACKED_FLOAT3 direction;   // Direction the ray is traveling.
    float maxDistance;         // Maximum allowed distance for this ray.
    PACKED_FLOAT3 color;       // The accumulated color along the ray's path so far.
    float sequenceIndexOffset; // The offset into the sequence for this ray.
};
