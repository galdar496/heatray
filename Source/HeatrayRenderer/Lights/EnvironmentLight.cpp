#include "EnvironmentLight.h"

#include <RLWrapper/Shader.h>
#include <RLWrapper/Program.h>
#include <RLWrapper/Texture.h>
#include <Utility/ShaderCodeLoader.h>
#include <Utility/TextureLoader.h>

#include <glm/glm/glm.hpp>

#include <assert.h>

EnvironmentLight::EnvironmentLight()
{
	// Setup the environment light OpenRL data.

	// Shader.
	std::shared_ptr<openrl::Shader> rayShader = nullptr;
	std::shared_ptr<openrl::Shader> vertexShader = nullptr;
	{
		std::vector<std::string> shaderSource;

		// Vertex.
		util::loadShaderSourceFile("passthrough.rlsl", shaderSource);
		vertexShader = openrl::Shader::createFromMultipleStrings(shaderSource, openrl::Shader::ShaderType::kVertex, "Environment Light Vertex Shader");
		assert(vertexShader);

		shaderSource.clear();

		// Ray.
		util::loadShaderSourceFile("environmentLight.rlsl", shaderSource);
		rayShader = openrl::Shader::createFromMultipleStrings(shaderSource, openrl::Shader::ShaderType::kRay, "Environment Light Ray Shader");
		assert(rayShader);
	}

	// Program.
	{
		m_program = openrl::Program::create();
		m_program->attach(vertexShader, openrl::Shader::ShaderType::kRay);
		m_program->attach(rayShader, openrl::Shader::ShaderType::kVertex);
		bool linked = m_program->link("Environment Light Program");
		assert(linked);
	}

	// Now create the primitive and associate the program with it.
	m_primitive = openrl::Primitive::create();
	m_primitive->attachProgram(m_program);

	// Initialize the uniforms to a default state.
	setUniforms();
}

void EnvironmentLight::changeImageSource(const char* path, bool builtInMap)
{
	assert(path);

	std::string fullPath;
	if (builtInMap) {
		static constexpr char *BASE_PATH = "Resources/Environments/";
		fullPath = std::string(BASE_PATH) + std::string(path);
	} else {
		fullPath = std::string(path);
	}

	if (m_textureSourcePath != fullPath) {
		m_texture = util::loadTexture(fullPath.c_str());
		setUniforms();
		m_textureSourcePath = fullPath;
	}
}

void EnvironmentLight::enableWhiteFurnaceTest()
{
	static constexpr const char * WHITE_FURNACE = "white furnace test";
	if (m_textureSourcePath != std::string(WHITE_FURNACE)) {
		// Load a white furnace texture. Set to 0.8 instead of full white so that it's obvious if there is more energy being emitted
		// by the surface than should be.
		openrl::Texture::Descriptor desc;
		desc.dataType = RL_FLOAT;
		desc.format = RL_RGB;
		desc.internalFormat = RL_RGB;
		desc.width = 1;
		desc.height = 1;

		openrl::Texture::Sampler sampler;
		sampler.magFilter = RL_LINEAR;
		sampler.minFilter = RL_LINEAR;
		sampler.wrapS = RL_CLAMP_TO_EDGE;
		sampler.wrapT = RL_CLAMP_TO_EDGE;

		glm::vec3 data = glm::vec3(0.8f);
		m_texture = openrl::Texture::create(&data.x, desc, sampler, false);
		setUniforms();

		m_textureSourcePath = std::string(WHITE_FURNACE);
	}
}

void EnvironmentLight::rotate(const float theta_radians)
{
	if (m_thetaRotation != theta_radians) {
		m_thetaRotation = theta_radians;
		setUniforms();
	}
}

void EnvironmentLight::setExposure(const float exposureCompensation)
{
	if (m_exposureCompensation != exposureCompensation) {
		m_exposureCompensation = exposureCompensation;
		setUniforms();
	}
}

void EnvironmentLight::setUniforms() const
{
	assert(m_primitive);
	assert(m_program);

	m_primitive->bind();
	m_program->bind();
	m_program->set1f(m_program->getUniformLocation("exposureCompensation"), std::powf(2.0f, m_exposureCompensation));
	m_program->set1f(m_program->getUniformLocation("thetaRotation"), m_thetaRotation);

	if (m_texture) {
		m_program->setTexture(m_program->getUniformLocation("environmentTexture"), m_texture);
	}
	m_primitive->unbind();
}