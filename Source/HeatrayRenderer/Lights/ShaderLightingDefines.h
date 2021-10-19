﻿//
//  ShaderLightingDefines.h
//  Heatray
//
//  These structures match the lighting structures defined in OpenRL shaders.
//

#pragma once

#include <RLWrapper/Primitive.h>

#define GLM_SWIZZLE
#include <glm/glm/glm.hpp>

#include <sstream>

struct ShaderLightingDefines {
	static constexpr size_t MAX_NUM_DIRECTIONAL_LIGHTS = 5;

	static void appendLightingShaderDefines(std::stringstream& stream)
	{
		stream << "#define MAX_NUM_DIRECTIONAL_LIGHTS " << MAX_NUM_DIRECTIONAL_LIGHTS << std::endl;
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