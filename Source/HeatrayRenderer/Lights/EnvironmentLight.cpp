#include "EnvironmentLight.h"

#include <RLWrapper/Buffer.h>
#include <RLWrapper/Shader.h>
#include <RLWrapper/Program.h>
#include <RLWrapper/Texture.h>
#include <Utility/ShaderCodeLoader.h>
#include <Utility/TextureLoader.h>

#include <glm/glm/glm.hpp>

#include <assert.h>
#include <sstream>

EnvironmentLight::EnvironmentLight(const std::string_view name, std::shared_ptr<openrl::Buffer> lightBuffer)
: Light(name, Light::Type::kEnvironment)
{
    // Setup the environment light OpenRL data.

    std::stringstream defines;
    ShaderLightingDefines::appendLightingShaderDefines(defines);
    m_program = util::buildProgram("passthrough.rlsl", "environmentLight.rlsl", "Environment Light", defines.str());
    assert(m_program);

    // Create the primitive and associate the program with it.
    m_primitive = openrl::Primitive::create();
    m_primitive->attachProgram(m_program);
}

void EnvironmentLight::changeImageSource(const std::string_view path, bool builtInMap)
{
    assert(!path.empty());

    std::string fullPath;
    if (builtInMap) {
        static constexpr std::string_view BASE_PATH = "Resources/Environments/";
        fullPath = std::string(BASE_PATH) + std::string(path);
    } else {
        fullPath = std::string(path);
    }

    if (m_textureSourcePath != fullPath) {
        m_texture = util::loadTexture(fullPath);
        m_textureSourcePath = fullPath;
    }
}

void EnvironmentLight::enableSolidColor(const glm::vec3& color)
{
    if ((m_textureSourcePath != SOLID_COLOR) ||
        (m_solidColor != color)) {
        // Load a solid color texture. Set to 0.8 instead of full white so that it's obvious if there is more energy being emitted
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

        m_texture = openrl::Texture::create(&color.x, desc, sampler, false);

        m_textureSourcePath = std::string(SOLID_COLOR);
        m_solidColor = color;
    }
}

void EnvironmentLight::rotate(const float theta_radians)
{
    m_thetaRotation = theta_radians;
}

void EnvironmentLight::setExposure(const float exposureCompensation)
{
    m_exposureCompensation = exposureCompensation;
}

void EnvironmentLight::copyToLightBuffer(EnvironmentLightBuffer* buffer)
{
    assert(buffer);
    assert(m_primitive);
    assert(m_program);

    if (m_texture) {
        buffer->texture = m_texture->texture();
    } else {
        buffer->texture = RL_NULL_TEXTURE;
    }
    buffer->exposureCompensation = std::powf(2.0f, m_exposureCompensation);
    buffer->thetaRotation = m_thetaRotation;
    buffer->primitive = m_primitive->primitive();
}
