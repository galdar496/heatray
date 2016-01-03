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
    
    typedef std::vector<math::vec3f> Positions;
    
    math::vec3f dimensions[2];    // Dimensions of the area light. [0]=lower-left corner, [1]=upper-right corner.
    math::vec3f normal;           // Normal of this light's geometry.
    RLprimitive primitive;        // Primitive which represents the light.
	Positions   sample_positions; // Randomized sample points on the light. Each point is used for a single pass.
};
