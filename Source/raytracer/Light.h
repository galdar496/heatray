//
//  Light.h
//  Heatray
//
//  Created by Cody White on 8/6/14.
//
//

#pragma once

#include <OpenRL/rl.h>
#include "../math/Vector.h"
#include <vector>

/// Object which stores the properties of a single light.
struct Light
{
    Light() : primitive(RL_NULL_PRIMITIVE) {}
    
    typedef std::vector<math::vec3f> Points;
    
    RLprimitive primitive;        // Primitive which represents the light.
    Points   sample_positions;    // Randomized sample points on the light. Each point is used for a single pass.
    Points   sample_normals;      // Normals for each sample point on the light.
};
