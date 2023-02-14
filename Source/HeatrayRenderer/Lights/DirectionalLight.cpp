#include "DirectionalLight.h"

#include <RLWrapper/Shader.h>
#include <RLWrapper/Program.h>
#include <Utility/ShaderCodeLoader.h>
#include <Utility/StringUtils.h>

#include <glm/glm/glm.hpp>
#include <glm/glm/gtx/quaternion.hpp>

#include <assert.h>
#include <cstring>
#include <sstream>

static constexpr float WATTS_TO_LUMENS = 683.0f;
static constexpr float LUMENS_TO_WATTS = 1.0f / 683.0f;

DirectionalLight::DirectionalLight(const std::string& name, size_t lightIndex, std::shared_ptr<openrl::Buffer> lightBuffer)
: Light(name, Light::Type::kDirectional)
, m_lightIndex(lightIndex)
{
    // Setup the environment light OpenRL data.

    std::stringstream defines;
    ShaderLightingDefines::appendLightingShaderDefines(defines);
    m_program = util::buildProgram("passthrough.rlsl", "directionalLight.rlsl", "DirectionalLight", defines.str());
    assert(m_program);

    // Create the primitive and associate the program with it.
    m_primitive = openrl::Primitive::create();
    m_primitive->attachProgram(m_program);

    // Initialize the uniforms to a default state.
    setUniforms();

    m_params.color = glm::vec3(1.0f);
    m_params.illuminance = WATTS_TO_LUMENS * glm::pi<float>(); // We specify a default intensity of 1 watt * π.
    m_params.orientation.phi = 0.0f;
    m_params.orientation.theta = glm::half_pi<float>();
}

void DirectionalLight::copyToLightBuffer(DirectionalLightsBuffer* buffer)
{
    assert(buffer);
    
    buffer->directions[m_lightIndex] = calculateDirection();
    buffer->primitives[m_lightIndex] = m_primitive->primitive();

    // We convert from photometric to radiometric units for the shader.
    buffer->colors[m_lightIndex] = m_params.color * (m_params.illuminance * LUMENS_TO_WATTS);
}

void DirectionalLight::setParams(const Params &params)
{
    m_params = params;
}

void DirectionalLight::updateLightIndex(const size_t newLightIndex)
{
    m_lightIndex = newLightIndex;
    setUniforms();
}

glm::vec3 DirectionalLight::calculateDirection()
{
    // Calculate the directional vector.
    constexpr glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
    constexpr glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::quat rotTheta = glm::angleAxis(m_params.orientation.theta, right);
    glm::quat rotPhi = glm::angleAxis(m_params.orientation.phi, up);
    glm::quat orientation = rotTheta * rotPhi;

    const glm::mat4 viewMatrix = glm::mat4_cast(glm::inverse(orientation));

    glm::vec3 direction = viewMatrix[2].xyz() * -1.0f;
    return glm::normalize(direction) * -1.0f;
}

void DirectionalLight::setUniforms() const
{
    assert(m_primitive);
    assert(m_program);

    m_primitive->bind();
    m_program->bind();
    m_program->set1i(m_program->getUniformLocation("lightIndex"), (int)m_lightIndex);
    m_primitive->unbind();
}
