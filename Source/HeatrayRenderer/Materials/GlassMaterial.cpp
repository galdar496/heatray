#include "GlassMaterial.h"
#include "../Lights/ShaderLightingDefines.h"

#include <RLWrapper/Shader.h>
#include <Utility/Log.h>
#include <Utility/ShaderCodeLoader.h>

#include <glm/glm/gtc/color_space.hpp>
#include <glm/glm/gtx/color_space.hpp>
#include <glm/glm/gtx/compatibility.hpp>

#include <assert.h>
#include <sstream>

struct ShaderParams
{
    // RLtextures need to come first, since they require 8 byte alignment.
    RLtexture baseColorTexture = RL_NULL_TEXTURE;
    RLtexture normalmap = RL_NULL_TEXTURE;
    RLtexture metallicRoughnessTexture = RL_NULL_TEXTURE; // B: metallic, G: roughness

    glm::vec3 baseColor = glm::vec3(1.0f);
    float roughness = 0.0f;
    float roughnessAlpha = 0.0f; ///< GGX alpha value (roughness^2).
    float ior = 1.0f;
    float density = 0.0f;
    float specularF0 = 0.0f;
};

void GlassMaterial::build()
{
    m_dummyTexture = openrl::Texture::getDummyTexture();

    // Load the parameters into the uniform block buffer.
    assert(m_constants == nullptr);
    m_constants = openrl::Buffer::create(RL_UNIFORM_BLOCK_BUFFER, nullptr, sizeof(ShaderParams), "Glass uniform block");
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
        if (m_params.normalmap) {
            hasTextures = true;
            hasNormalmap = true;
            shaderPrefix << "#define HAS_NORMALMAP\n";
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
    m_program = util::buildProgram(m_vertexShader, m_shader, "Glass", shaderPrefix.str());

    // NOTE: the association of the program and the uniform block needs to happen in the calling code.
    // This is because there is no RLprimitive at the material level to properly bind.
}

void GlassMaterial::rebuild()
{
    m_program.reset();
    m_constants.reset();

    build();
}

void GlassMaterial::modify()
{
    // Convert the parameters into what the shader expects and compute some new values
    // based on the parameters.
    ShaderParams shaderParams;

    constexpr float kMinRoughness = 0.01f; // Too low of a roughness will force a dirac delta response which will cause math errors in the shader code.

    shaderParams.baseColor = glm::saturate(m_params.baseColor);
    shaderParams.roughness = glm::max(glm::saturate<float, glm::defaultp>(m_params.roughness), kMinRoughness);
    shaderParams.roughnessAlpha = shaderParams.roughness * shaderParams.roughness;
    shaderParams.density = glm::saturate<float, glm::defaultp>(m_params.density);
    shaderParams.ior = glm::max(0.0f, m_params.ior);

    // Calculate F0 based on the IOR.
    float F0 = glm::abs<float>((1.0f - shaderParams.ior) / (1.0f + shaderParams.ior));
    shaderParams.specularF0 = F0 * F0;

    if (m_params.baseColorTexture) {
        shaderParams.baseColorTexture = m_params.baseColorTexture->texture();
    }
    else {
        shaderParams.baseColorTexture = m_dummyTexture->texture();
    }
    if (m_params.normalmap) {
        shaderParams.normalmap = m_params.normalmap->texture();
    }
    else {
        shaderParams.normalmap = m_dummyTexture->texture();
    }
    if (m_params.metallicRoughnessTexture) {
        shaderParams.metallicRoughnessTexture = m_params.metallicRoughnessTexture->texture();
    }
    else {
        shaderParams.metallicRoughnessTexture = m_dummyTexture->texture();
    }

    m_constants->modify(&shaderParams, sizeof(ShaderParams));
}
