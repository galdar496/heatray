//
//  Math.h
//  Heatray5 macOS
//
//  Created by Cody White on 4/7/23.
//

#pragma once

#include <simd/simd.h>

namespace util {

namespace constants {

constexpr float PI = M_PI;
constexpr float TWO_PI = 2.0f * PI;
constexpr float HALF_PI = PI * 0.5f;

} // namespace constants.

float degrees(const float radians) {
    return (radians * (180.0f / constants::PI));
}

float radians(const float degrees) {
    return (degrees * (constants::PI / 180.0f));
}

} // namespace util.
