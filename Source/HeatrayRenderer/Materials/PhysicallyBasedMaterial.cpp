#include "PhysicallyBasedMaterial.h"
#include "MultiScatterUtil.h"
#include "../Lights/ShaderLightingDefines.h"

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
    RLtexture baseColorTexture = RL_NULL_TEXTURE;
    RLtexture metallicRoughnessTexture = RL_NULL_TEXTURE; // B: metallic, G: roughness
    RLtexture emissiveTexture = RL_NULL_TEXTURE;
    RLtexture normalmap = RL_NULL_TEXTURE;
    RLtexture clearCoatTexture = RL_NULL_TEXTURE;
    RLtexture clearCoatRoughnessTexture = RL_NULL_TEXTURE;
    RLtexture clearCoatNormalmap = RL_NULL_TEXTURE;
    RLtexture multiscatterLUT = RL_NULL_TEXTURE;

    glm::vec3 baseColor = glm::vec3(1.0f);
    glm::vec3 emissiveColor = glm::vec3(0.0f);
    float metallic = 0.0f;
    float roughness = 0.0f;
    float specularF0 = 0.0f;
    float roughnessAlpha = 0.0f; ///< GGX alpha value (roughness^2).
    float clearCoat = 0.0f;
    float clearCoatRoughness = 0.0f;
    float clearCoatRoughnessAlpha = 0.0f; ///< GGX alpha value (roughness^2).
};

void PhysicallyBasedMaterial::build()
{
    m_multiscatterLUT = loadMultiscatterTexture();
    m_dummyTexture = openrl::Texture::getDummyTexture();

    // Load the parameters into the uniform block buffer.
    assert(m_constants == nullptr);
    m_constants = openrl::Buffer::create(RL_UNIFORM_BLOCK_BUFFER, nullptr, sizeof(ShaderParams), "PhysicallyBased uniform block");
    modify();
    assert(m_constants->valid());

    std::stringstream shaderPrefix;
    bool hasTextures = false;
    bool hasNormalmap = false;

    // Defines for lighting.
    ShaderLightingDefines::appendLightingShaderDefines(shaderPrefix);

    // Add shader defines based on detected features.
    {
        if (m_params.baseColorTexture || m_params.forceEnableAllTextures) {
            hasTextures = true;
            shaderPrefix << "#define HAS_BASE_COLOR_TEXTURE\n";
        }
        if (m_params.metallicRoughnessTexture || m_params.forceEnableAllTextures) {
            hasTextures = true;
            shaderPrefix << "#define HAS_METALLIC_ROUGHNESS_TEXTURE\n";
        }
        if (m_params.emissiveTexture) {
            hasTextures = true;
            shaderPrefix << "#define HAS_EMISSIVE_TEXTURE\n";
        }
        if (m_params.normalmap) {
            hasTextures = true;
            hasNormalmap = true;
            shaderPrefix << "#define HAS_NORMALMAP\n";
        }
        if (m_params.clearCoatTexture || m_params.forceEnableAllTextures) {
            hasTextures = true;
            shaderPrefix << "#define HAS_CLEARCOAT_TEXTURE\n";
        }
        if (m_params.clearCoatRoughnessTexture || m_params.forceEnableAllTextures) {
            hasTextures = true;
            shaderPrefix << "#define HAS_CLEARCOAT_ROUGHNESS_TEXTURE\n";
        }
        if (m_params.clearCoatNormalmap) {
            hasTextures = true;
            hasNormalmap = true;
            shaderPrefix << "#define HAS_CLEARCOAT_NORMALMAP\n";
        }
    }

    // Loadup the shader code.
    // TODO: this should use some kind of shader cache.
    if (hasTextures) {
        if (hasNormalmap) {
            shaderPrefix << "#define USE_TANGENT_SPACE\n";
        }
        shaderPrefix << "#define HAS_TEXTURES\n";
    }

    LOG_INFO("Building shader: %s with flags:\n%s", m_shader, shaderPrefix.str().c_str());
    m_program = util::buildProgram(m_vertexShader, m_shader, "PhysicallyBased", shaderPrefix.str());

    // NOTE: the association of the program and the uniform block needs to happen in the calling code.
    // This is because there is no RLprimitive at the material level to properly bind.
}

void PhysicallyBasedMaterial::rebuild()
{
    m_program.reset();
    m_constants.reset();

    build();
}

void PhysicallyBasedMaterial::modify()
{
    // Convert the parameters into what the shader expects and compute some new values
    // based on the parameters.  
    ShaderParams shaderParams;

    constexpr float kMinRoughness = 0.01f; // Too low of a roughness will force a dirac delta response which will cause math errors in the shader code.
    constexpr float kMaxSpecularF0 = 0.08f;  // As suggested by Burley.
    constexpr float kMaxClearcoat = 0.2f; // As suggested by Burley.

    shaderParams.baseColor = glm::saturate(m_params.baseColor);
    shaderParams.emissiveColor = glm::saturate(m_params.emissiveColor);
    shaderParams.metallic = glm::saturate<float, glm::defaultp>(m_params.metallic);
    shaderParams.roughness = glm::max(glm::saturate<float, glm::defaultp>(m_params.roughness), kMinRoughness);
    shaderParams.specularF0 = m_params.specularF0 * kMaxSpecularF0;
    shaderParams.roughnessAlpha = shaderParams.roughness * shaderParams.roughness;
    shaderParams.clearCoat = m_params.clearCoat * kMaxClearcoat;
    shaderParams.clearCoatRoughness = glm::max(glm::saturate<float, glm::defaultp>(m_params.clearCoatRoughness), kMinRoughness);
    shaderParams.clearCoatRoughnessAlpha = shaderParams.clearCoatRoughness * shaderParams.clearCoatRoughness;

    if (m_params.baseColorTexture) {
        shaderParams.baseColorTexture = m_params.baseColorTexture->texture();
    }
    else {
        shaderParams.baseColorTexture = m_dummyTexture->texture();
    }
    if (m_params.metallicRoughnessTexture) {
        shaderParams.metallicRoughnessTexture = m_params.metallicRoughnessTexture->texture();
    }
    else {
        shaderParams.metallicRoughnessTexture = m_dummyTexture->texture();
    }
    if (m_params.emissiveTexture) {
        shaderParams.emissiveTexture = m_params.emissiveTexture->texture();
    } else {
        shaderParams.emissiveTexture = m_dummyTexture->texture();
    }
    if (m_params.normalmap) {
        shaderParams.normalmap = m_params.normalmap->texture();
    } else {
        shaderParams.normalmap = m_dummyTexture->texture();
    }
    if (m_params.clearCoatTexture) {
        shaderParams.clearCoatTexture = m_params.clearCoatTexture->texture();
    }
    else {
        shaderParams.clearCoatTexture = m_dummyTexture->texture();
    }
    if (m_params.clearCoatRoughnessTexture) {
        shaderParams.clearCoatRoughnessTexture = m_params.clearCoatRoughnessTexture->texture();
    }
    else {
        shaderParams.clearCoatRoughnessTexture = m_dummyTexture->texture();
    }
    if (m_params.clearCoatNormalmap) {
        shaderParams.clearCoatNormalmap = m_params.clearCoatNormalmap->texture();
    }
    else {
        shaderParams.clearCoatNormalmap = m_dummyTexture->texture();
    }

    shaderParams.multiscatterLUT = m_multiscatterLUT->texture();

    m_constants->modify(&shaderParams, sizeof(ShaderParams));
}