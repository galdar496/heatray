#include "PointLight.h"

#include <RLWrapper/Shader.h>
#include <RLWrapper/Program.h>
#include <Utility/ShaderCodeLoader.h>
#include <Utility/StringUtils.h>

#include <glm/glm/glm.hpp>
#include <glm/glm/ext/scalar_constants.hpp>

#include <assert.h>
#include <cstring>
#include <sstream>

static constexpr float WATTS_TO_LUMENS = 683.0f;
static constexpr float LUMENS_TO_WATS = 1.0f / 683.0f;

PointLight::PointLight(size_t lightIndex, std::shared_ptr<openrl::Buffer> lightBuffer)
: m_lightIndex(lightIndex)
{
    // Setup the environment light OpenRL data.

    std::stringstream defines;
    ShaderLightingDefines::appendLightingShaderDefines(defines);
    m_program = util::buildProgram("passthrough.rlsl", "pointLight.rlsl", "PointLight", defines.str());
    assert(m_program);

    // Create the primitive and associate the program with it.
    m_primitive = openrl::Primitive::create();
    m_primitive->attachProgram(m_program);

    // Initialize the uniforms to a default state.
    setUniforms();

    m_params.color = glm::vec3(1.0f);
    m_params.intensity = WATTS_TO_LUMENS * (4.0f * (glm::pi<float>() * glm::pi<float>())); // We specify a default intensity of 1 watt * 4π^2.
    m_params.position = glm::vec3(0.0f, 0.0f, 0.0f);
}

void PointLight::copyToLightBuffer(PointLightsBuffer* buffer)
{
    assert(buffer);
    
    buffer->positions[m_lightIndex] = m_params.position;
    // We convert from photometric to radiometric units for the shader.
    buffer->colors[m_lightIndex] = m_params.color * (m_params.intensity / (4.0f * glm::pi<float>())) * LUMENS_TO_WATS;
    buffer->primitives[m_lightIndex] = m_primitive->primitive();
}

void PointLight::updateLightIndex(const size_t newLightIndex)
{
    m_lightIndex = newLightIndex;
    setUniforms();
}

void PointLight::setUniforms() const
{
    assert(m_primitive);
    assert(m_program);

    m_primitive->bind();
    m_program->bind();
    m_program->set1i(m_program->getUniformLocation("lightIndex"), m_lightIndex);
    m_primitive->unbind();
}