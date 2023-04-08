//
//  Display.metal
//  Heatray5 macOS
//
//  Created by Cody White on 3/25/23.
//

#import "DisplayShaderTypes.h"

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct DisplayVertexOut {
    float4 position [[position]];
    float2 uv;
};

vertex DisplayVertexOut DisplayVS(unsigned short vertexId [[vertex_id]],
                                  constant DisplayVertexShader::Constants& constants [[ buffer(DisplayVertexShader::BufferLocation) ]]) {
    const float2 quadCorners[4] = {
        float2(constants.quadOffset, -1.0f),
        float2(+1.0f,                -1.0f),
        float2(constants.quadOffset, +1.0f),
        float2(+1.0f,                +1.0f)
    };
    const float2 uvs[4] = {
        float2(0.0f, 0.0f),
        float2(1.0f, 0.0f),
        float2(0.0f, 1.0f),
        float2(1.0f, 1.0f)
    };

    DisplayVertexOut out;
    
    out.position = float4(quadCorners[vertexId], 0, 1);
    out.uv = uvs[vertexId];
    
    return out;
}

fragment float4 DisplayFS(DisplayVertexOut in [[stage_in]],
                          texture2d<float> raytracedPixels)
{
    constexpr sampler sampler(min_filter::nearest, mag_filter::nearest, mip_filter::none);

    float4 color = raytracedPixels.sample(sampler, in.uv);

    // The 'w' component holds the number of samples for this pixel.
    return float4(color.xyz / max(1.0f, color.w), 1.0f);
    
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
}
