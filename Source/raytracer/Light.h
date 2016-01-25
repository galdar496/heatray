//
//  Light.h
//  Heatray
//
//  Created by Cody White on 8/6/14.
//
//

#pragma once

#include "../math/Vector.h"
#include <OpenRL/rl.h>
#include <vector>

///
/// Object which stores the properties of a single light.
///

struct Light
{
    Light() : primitive(RL_NULL_PRIMITIVE) {}
    
    typedef std::vector<math::Vec3f> Points;
    
    RLprimitive primitive;        ///< Primitive which represents the light.
    Points   samplePositions;     ///< Randomized sample points on the light. Each point is used for a single pass.
    Points   sampleNormals;       ///< Normals for each sample point on the light.
};
