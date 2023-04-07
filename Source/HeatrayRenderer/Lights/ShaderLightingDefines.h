//
//  ShaderLightingDefines.h
//  Heatray
//
//  These structures match the lighting structures defined in OpenRL shaders.
//

#pragma once

#if 0

#include <RLWrapper/Primitive.h>

#define GLM_FORCE_SWIZZLE
#include <glm/glm/glm.hpp>

#include <sstream>
 
struct ShaderLightingDefines {
    static constexpr size_t MAX_NUM_DIRECTIONAL_LIGHTS = 5;
    static constexpr size_t MAX_NUM_POINT_LIGHTS = 5;
    static constexpr size_t MAX_NUM_SPOT_LIGHTS = 5;

    //-------------------------------------------------------------------------
    // Every compiled shader should call this function to get the proper defines
    // for lighting.
    static void appendLightingShaderDefines(std::stringstream& stream)
    {
        stream << "#define MAX_NUM_DIRECTIONAL_LIGHTS " << MAX_NUM_DIRECTIONAL_LIGHTS << std::endl;
        stream << "#define MAX_NUM_POINT_LIGHTS " << MAX_NUM_POINT_LIGHTS << std::endl;
        stream << "#define MAX_NUM_SPOT_LIGHTS " << MAX_NUM_SPOT_LIGHTS << std::endl;
    }
};

struct EnvironmentLightBuffer {
    RLtexture texture = RL_NULL_TEXTURE;
    float exposureCompensation = 0.0f;
    float thetaRotation = 0.0f; // In radians.
    RLprimitive primitive = RL_NULL_PRIMITIVE;
};

struct DirectionalLightsBuffer {
    int numberOfLights = 0;

    glm::vec3 directions[ShaderLightingDefines::MAX_NUM_DIRECTIONAL_LIGHTS]; // Direction TO the light.
    glm::vec3 colors[ShaderLightingDefines::MAX_NUM_DIRECTIONAL_LIGHTS]; // In Radiometric units.
    RLprimitive primitives[ShaderLightingDefines::MAX_NUM_DIRECTIONAL_LIGHTS] = { RL_NULL_PRIMITIVE };
};

struct PointLightsBuffer {
    int numberOfLights = 0;

    glm::vec3 positions[ShaderLightingDefines::MAX_NUM_POINT_LIGHTS]; // World-space positions of the lights.
    glm::vec3 colors[ShaderLightingDefines::MAX_NUM_POINT_LIGHTS]; // In Radiometric units.
    RLprimitive primitives[ShaderLightingDefines::MAX_NUM_POINT_LIGHTS] = { RL_NULL_PRIMITIVE };
};

struct SpotLightsBuffer {
    int numberOfLights = 0;

    glm::vec3 positions[ShaderLightingDefines::MAX_NUM_SPOT_LIGHTS]; // World-space positions of the lights.
    glm::vec3 directions[ShaderLightingDefines::MAX_NUM_SPOT_LIGHTS]; // Direction from the light.
    glm::vec3 colors[ShaderLightingDefines::MAX_NUM_SPOT_LIGHTS]; // In Radiometric units.
    glm::vec2 angles[ShaderLightingDefines::MAX_NUM_SPOT_LIGHTS]; // x: inner, y: outer.
    RLprimitive primitives[ShaderLightingDefines::MAX_NUM_SPOT_LIGHTS] = { RL_NULL_PRIMITIVE };
};

#endif
