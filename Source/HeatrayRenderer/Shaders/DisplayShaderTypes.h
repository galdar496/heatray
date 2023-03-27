//
//  DisplayShaderTypes.h
//  Heatray5
//
//  Created by Cody White on 3/25/23.
//

#pragma once

#ifdef __METAL_VERSION__
    #define CONSTANT constant
#else
    #define CONSTANT static constexpr
#endif

namespace DisplayVertexShader {

struct Constants {
    // Offset to start rendering to within the view (so that we don't render over the UI).
    float quadOffset = 0.0f;
};

CONSTANT uint32_t BufferLocation = 0;

} // namespace DisplayVertexShader.

namespace DisplayFragmentShader {

} // namespace DisplayFragmentShader.

#undef CONSTANT
