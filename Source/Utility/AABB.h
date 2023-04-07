//
//  AABB.h
//  Heatray
//
//  Represents a simple axis-aligned box.
//
//

#pragma once

#include <simd/simd.h>
#include <cmath>
#include <limits>

namespace util {

struct AABB {
    AABB() {
        min = simd::float3(std::numeric_limits<float>::max());
        max = simd::float3(std::numeric_limits<float>::min());
    }
    ~AABB() = default;
    AABB(const AABB& other) = default;
    AABB& operator=(const AABB& other) = default;

    //-------------------------------------------------------------------------
    // Expand the AABB to contain the new point.
    void expand(const simd::float3 v) {
        min = vector_min(min, v);
        max = vector_max(max, v);
    }

    vector_float3 center() const {
        simd::float3 transformedMin = matrix_multiply(transform, simd::make_float4(min, 1.0f)).xyz;
        simd::float3 transformedMax = (transform * simd::make_float4(max, 1.0f)).xyz;
        return (transformedMin + transformedMax) * 0.5f;
    }

    float radius() const {
        simd::float3 transformedMin = (transform * simd::make_float4(min, 1.0f)).xyz;
        simd::float3 transformedMax = (transform * simd::make_float4(max, 1.0f)).xyz;
        return simd::distance(transformedMax, transformedMin);
    }
    
    float bottom() const {
        simd::float3 transformedMin = (transform * simd::make_float4(min, 1.0f)).xyz;
        simd::float3 transformedMax = (transform * simd::make_float4(max, 1.0f)).xyz;
        float floor = center().y - std::fabsf(transformedMax.y - transformedMin.y) * 0.5f;
        return floor;
    }
    
    bool valid() const {
        return simd::any(min < max);
    }

    // Corners of the box.
    simd::float3 min;
    simd::float3 max;
    
    simd::float4x4 transform = matrix_identity_float4x4;
};

} // namespace util.
