#include "SceneLighting.h"

#include "ShaderLightingDefines.h"

#include <RLWrapper/Program.h>
#include <Utility/Log.h>

SceneLighting::SceneLighting()
{
    m_environment.buffer = openrl::Buffer::create(RL_ARRAY_BUFFER, nullptr, sizeof(EnvironmentLightBuffer), "Environment Light Buffer");
    m_directional.buffer = openrl::Buffer::create(RL_ARRAY_BUFFER, nullptr, sizeof(DirectionalLightsBuffer), "Directional Lights Buffer");

    // Sets all lighting buffers to good defaults.
    clear();
}

void SceneLighting::clear()
{
    // Environment Light.
    removeEnvironmentLight();

    // Directional Lights.
    {
        for (size_t iLight = 0; iLight < ShaderLightingDefines::MAX_NUM_DIRECTIONAL_LIGHTS; ++iLight) {
            m_directional.lights[iLight].reset();
        }

        m_directional.buffer->bind();
        DirectionalLightsBuffer* directionalLights = m_directional.buffer->mapBuffer<DirectionalLightsBuffer>();
        directionalLights->numberOfLights = 0;
        m_directional.buffer->unmapBuffer();
        m_directional.buffer->unbind();

        m_directional.count = 0;
    }
}

void SceneLighting::bindLightingBuffersToProgram(std::shared_ptr<openrl::Program> program)
{
    // Environment Light.
    {
        RLint blockIndex = program->getUniformBlockIndex("EnvironmentLight");
        if (blockIndex != -1) {
            program->setUniformBlock(blockIndex, m_environment.buffer->buffer());
        }
    }

    // Directional lights.
    {
        RLint blockIndex = program->getUniformBlockIndex("DirectionalLights");
        if (blockIndex != -1) {
            program->setUniformBlock(blockIndex, m_directional.buffer->buffer());
        }
    }
}

std::shared_ptr<EnvironmentLight> SceneLighting::addEnvironmentLight()
{
    m_environment.light = std::shared_ptr<EnvironmentLight>(new EnvironmentLight(m_environment.buffer));

    m_environment.light->primitive()->bind();
    m_environment.light->program()->bind();
    bindLightingBuffersToProgram(m_environment.light->program());
    m_environment.light->primitive()->unbind();

    updateLight(m_environment.light);

    return m_environment.light;
}

void SceneLighting::removeEnvironmentLight()
{
    m_environment.light.reset();

    m_environment.buffer->bind();
    EnvironmentLightBuffer* environmentLight = m_environment.buffer->mapBuffer<EnvironmentLightBuffer>();
    environmentLight->primitive = RL_NULL_PRIMITIVE;
    environmentLight->texture = RL_NULL_TEXTURE;
    m_environment.buffer->unmapBuffer();
    m_environment.buffer->unbind();
}

void SceneLighting::updateLight(std::shared_ptr<EnvironmentLight> light)
{
    m_environment.buffer->bind();
    EnvironmentLightBuffer* environmentLight = m_environment.buffer->mapBuffer<EnvironmentLightBuffer>();
    light->copyToLightBuffer(environmentLight);
    m_environment.buffer->unmapBuffer();
    m_environment.buffer->unbind();
}

std::shared_ptr<DirectionalLight> SceneLighting::addDirectionalLight()
{
    if (m_directional.count < ShaderLightingDefines::MAX_NUM_DIRECTIONAL_LIGHTS) {
        size_t lightIndex = m_directional.count;

        std::shared_ptr<DirectionalLight> light = std::shared_ptr<DirectionalLight>(new DirectionalLight(lightIndex, m_directional.buffer));
        m_directional.lights[lightIndex] = light;

        light->primitive()->bind();
        light->program()->bind();
        bindLightingBuffersToProgram(light->program());
        light->primitive()->unbind();

        ++m_directional.count;

        // Update the lighting buffer.
        updateLight(light);

        return light;
    }

    LOG_ERROR("Attempting to add too many Directional Lights!");
    return nullptr;
}

void SceneLighting::updateLight(std::shared_ptr<DirectionalLight> light)
{
    m_directional.buffer->bind();
    DirectionalLightsBuffer* directionalLights = m_directional.buffer->mapBuffer<DirectionalLightsBuffer>();
    light->copyToLightBuffer(directionalLights);
    directionalLights->numberOfLights = m_directional.count;
    m_directional.buffer->unmapBuffer();
    m_directional.buffer->unbind();
}
