//
//  ShaderMathConstants.h
//  Heatray5
//
//  Created by Cody White on 4/1/23.
//

#pragma once

#ifdef __METAL_VERSION__
    #define CONSTANT constant
#else
    #define CONSTANT static constexpr
#endif

namespace sm_constants {

CONSTANT float INFINITE_MAXT = -1.0f;

} // namespace constants.

#undef CONSTANT
