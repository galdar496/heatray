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

DirectionalLight::DirectionalLight(size_t lightIndex, std::shared_ptr<openrl::Buffer> lightBuffer)
: m_lightIndex(lightIndex)
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

	setOrientation(0.0f, glm::half_pi<float>());
	m_color = glm::vec3(1.0f);
	// We specify a default intensity of 683 * π.
	m_intensity = 683.0f * glm::pi<float>();
}

void DirectionalLight::copyToLightBuffer(DirectionalLightsBuffer* buffer)
{
	assert(buffer);
	
	buffer->directions[m_lightIndex] = m_direction * -1.0f;
	buffer->colors[m_lightIndex] = m_color * (m_intensity / 683.0f);
	buffer->primitives[m_lightIndex] = m_primitive->primitive();
}

void DirectionalLight::setOrientation(const float phi_radians, const float theta_radians)
{
	m_orientation.phi = phi_radians;
	m_orientation.theta = theta_radians;

	// Calculate the directional vector.
	{
		constexpr glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
		constexpr glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

		glm::quat rotTheta = glm::angleAxis(m_orientation.theta, right);
		glm::quat rotPhi = glm::angleAxis(m_orientation.phi, up);
		glm::quat orientation = rotTheta * rotPhi;

		const glm::mat4 viewMatrix = glm::mat4_cast(glm::inverse(orientation));

		m_direction = viewMatrix[2].xyz() * -1.0f;
		m_direction = glm::normalize(m_direction);
	}
}

void DirectionalLight::setUniforms() const
{
	assert(m_primitive);
	assert(m_program);

	m_primitive->bind();
	m_program->bind();
	m_program->set1i(m_program->getUniformLocation("lightIndex"), m_lightIndex);
	m_primitive->unbind();
}