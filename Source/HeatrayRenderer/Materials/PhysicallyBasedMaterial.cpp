#include "PhysicallyBasedMaterial.h"

#include <RLWrapper/Shader.h>
#include <Utility/Log.h>
#include <Utility/ShaderCodeLoader.h>

#include <glm/glm/gtc/color_space.hpp>
#include <glm/glm/gtx/color_space.hpp>
#include <glm/glm/gtx/compatibility.hpp>

#include <assert.h>
#include <sstream>

struct ShaderParams {
	// RLtextures need to come first, since they require 8 byte alignment.
	RLtexture baseColorTexture;
	RLtexture metallicRoughnessTexture; // R: metallic, G: roughness
	RLtexture emissiveTexture;
	RLtexture normalmap;
	RLtexture clearCoatTexture;
	RLtexture clearCoatRoughnessTexture;
	RLtexture clearCoatNormalmap;
	glm::vec3 baseColor;
	float metallic;
	float roughness;
	float specularF0;
	float roughnessAlpha; ///< GGX alpha value (roughness^2).
	float clearCoat;
	float clearCoatRoughness;
	float clearCoatRoughnessAlpha; ///< GGX alpha value (roughness^2).
};

void PhysicallyBasedMaterial::build(const PhysicallyBasedMaterial::Parameters& params)
{    
    // Load the parameters into the uniform block buffer.
	assert(m_constants.valid() == false);
	m_constants.setTarget(RL_UNIFORM_BLOCK_BUFFER);
	modify(params);
    assert(m_constants.valid());

	std::stringstream shaderPrefix;
	bool hasTextures = false;
	bool hasNormalmap = false;

	// Add shader defines based on detected features.
	{
		if (params.baseColorTexture) {
			hasTextures = true;
			shaderPrefix << "#define HAS_BASE_COLOR_TEXTURE\n";
		}
		if (params.metallicRoughnessTexture) {
			hasTextures = true;
			shaderPrefix << "#define HAS_METALLIC_ROUGHNESS_TEXTURE\n";
		}
		if (params.emissiveTexture) {
			hasTextures = true;
			shaderPrefix << "#define HAS_EMISSIVE_TEXTURE\n";
		}
		if (params.normalmap) {
			hasTextures = true;
			hasNormalmap = true;
			shaderPrefix << "#define HAS_NORMALMAP\n";
		}
		if (params.clearCoatTexture) {
			hasTextures = true;
			shaderPrefix << "#define HAS_CLEARCOAT_TEXTURE\n";
		}
		if (params.clearCoatRoughnessTexture) {
			hasTextures = true;
			shaderPrefix << "#define HAS_CLEARCOAT_ROUGHNESS_TEXTURE\n";
		}
		if (params.clearCoatNormalmap) {
			hasTextures = true;
			hasNormalmap = true;
			shaderPrefix << "#define HAS_CLEARCOAT_NORMALMAP\n";
		}
	}

    // Loadup the shader code.
    // TODO: this should use some kind of shader cache.
    char const * vertexShader;
    if (hasTextures) {
		if (hasNormalmap) {
			vertexShader = "positionNormalTexCoordTangentBitangent.vert.rlsl";
			shaderPrefix << "#define USE_TANGENT_SPACE\n";
		} else {
			vertexShader = "positionNormalTexCoord.vert.rlsl";
		}
		shaderPrefix << "#define HAS_TEXTURES\n";
    } else {
        vertexShader = "positionNormal.vert.rlsl";
    }

	LOG_INFO("Building shader: %s with flags: %s", m_shader, shaderPrefix.str().c_str());
    m_program = util::buildShader(vertexShader, m_shader, "PhysicallyBased", shaderPrefix.str());

    // NOTE: the association of the program and the uniform block needs to happen in the calling code.
    // This is because there is no RLprimitive at the material level to properly bind.
}

void PhysicallyBasedMaterial::modify(const PhysicallyBasedMaterial::Parameters& params)
{
	// Convert the parameters into what the shader expects and compute some new values
	// based on the parameters.  
	ShaderParams shaderParams;

	constexpr float kMinRoughness = 0.01f; // Too low of a roughness will force a dirac delta response which will cause math errors in the shader code.

	shaderParams.baseColor = glm::saturate(params.baseColor);
	shaderParams.metallic = glm::saturate<float, glm::defaultp>(params.metallic);
	shaderParams.roughness = glm::max(glm::saturate<float, glm::defaultp>(params.roughness), kMinRoughness);
	shaderParams.specularF0 = params.specularF0;
	shaderParams.roughnessAlpha = shaderParams.roughness * shaderParams.roughness;
	shaderParams.clearCoat = params.clearCoat;
	shaderParams.clearCoatRoughness = glm::max(glm::saturate<float, glm::defaultp>(params.clearCoatRoughness), kMinRoughness);
	shaderParams.clearCoatRoughnessAlpha = shaderParams.clearCoatRoughness * shaderParams.clearCoatRoughness;

	if (params.baseColorTexture) {
		shaderParams.baseColorTexture = params.baseColorTexture->texture();
	}
	else {
		shaderParams.baseColorTexture = openrl::getDummyTexture().texture();
	}
	if (params.metallicRoughnessTexture) {
		shaderParams.metallicRoughnessTexture = params.metallicRoughnessTexture->texture();
	}
	else {
		shaderParams.metallicRoughnessTexture = openrl::getDummyTexture().texture();
	}
	if (params.emissiveTexture) {
		shaderParams.emissiveTexture = params.emissiveTexture->texture();
	} else {
		shaderParams.emissiveTexture = openrl::getDummyTexture().texture();
	}
	if (params.normalmap) {
		shaderParams.normalmap = params.normalmap->texture();
	} else {
		shaderParams.normalmap = openrl::getDummyTexture().texture();
	}
	if (params.clearCoatTexture) {
		shaderParams.clearCoatTexture = params.clearCoatTexture->texture();
	}
	else {
		shaderParams.clearCoatTexture = openrl::getDummyTexture().texture();
	}
	if (params.clearCoatRoughnessTexture) {
		shaderParams.clearCoatRoughnessTexture = params.clearCoatRoughnessTexture->texture();
	}
	else {
		shaderParams.clearCoatRoughnessTexture = openrl::getDummyTexture().texture();
	}
	if (params.clearCoatNormalmap) {
		shaderParams.clearCoatNormalmap = params.clearCoatNormalmap->texture();
	}
	else {
		shaderParams.clearCoatNormalmap = openrl::getDummyTexture().texture();
	}

	m_constants.load(&shaderParams, sizeof(ShaderParams), "PhysicallyBased uniform block");
}