#include "GlassMaterial.h"

#include <RLWrapper/Shader.h>
#include <Utility/ShaderCodeLoader.h>

#include <glm/glm/gtc/color_space.hpp>
#include <glm/glm/gtx/color_space.hpp>
#include <glm/glm/gtx/compatibility.hpp>

#include <assert.h>
#include <iostream>

void GlassMaterial::build(const GlassMaterial::Parameters& params)
{
    // Convert the parameters into what the shader expects and compute some new values
    // based on the parameters.
    struct ShaderParams
    {
        glm::vec3 baseColor; // Linear version of the public parameter.
        float roughness;
        float roughnessAlpha; ///< GGX alpha value (roughness^2).
        float ior;
        float density;
        float specularF0;
    } shaderParams;

    constexpr float kMinRoughness = 0.01f; // Too low of a roughness will force a dirac delta response which will cause math errors in the shader code.

    shaderParams.baseColor      = glm::saturate(params.baseColor);
    shaderParams.roughness      = glm::max(glm::saturate<float, glm::defaultp>(params.roughness), kMinRoughness);
    shaderParams.roughnessAlpha = shaderParams.roughness * shaderParams.roughness;
    shaderParams.density        = glm::saturate<float, glm::defaultp>(params.density);
    shaderParams.ior            = glm::max(0.0f, params.ior);

    // Calculate F0 based on the IOR.
    float F0 = glm::abs<float>((1.0f - shaderParams.ior) / (1.0f + shaderParams.ior));
    shaderParams.specularF0 = F0 * F0;

    // Load the parameters into the uniform block buffer.
    assert(m_constants.valid() == false);
    m_constants.setTarget(RL_UNIFORM_BLOCK_BUFFER);
    m_constants.load(&shaderParams, sizeof(ShaderParams), "Glass uniform block");
    assert(m_constants.valid());

    // Loadup the shader code.
    // TODO: this should use some kind of shader cache.
    std::cout << "Building shader: " << m_shader << std::endl;
    m_program = util::buildShader(m_vertexShader, m_shader, "Glass");

    // NOTE: the association of the program and the uniform block needs to happen in the calling code.
    // This is because there is no RLprimitive at the material level to properly bind.
}