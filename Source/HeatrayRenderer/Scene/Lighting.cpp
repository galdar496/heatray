#include "Lighting.h"

#include <HeatrayRenderer/Lights/EnvironmentLight.h>
#include <HeatrayRenderer/Lights/DirectionalLight.h>
#include <HeatrayRenderer/Lights/PointLight.h>

#include <RLWrapper/Buffer.h>
#include <RLWrapper/Program.h>
#include <Utility/Log.h>

Lighting::Lighting()
{
    m_environment.buffer = openrl::Buffer::create(RL_ARRAY_BUFFER, nullptr, sizeof(EnvironmentLightBuffer), "Environment Light Buffer");
    m_directional.buffer = openrl::Buffer::create(RL_ARRAY_BUFFER, nullptr, sizeof(DirectionalLightsBuffer), "Directional Lights Buffer");
    m_point.buffer = openrl::Buffer::create(RL_ARRAY_BUFFER, nullptr, sizeof(PointLightsBuffer), "Point Lights Buffer");

    // Sets all lighting buffers to good defaults.
    clear();
}

void Lighting::clear()
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

    // Point Lights.
    {
        for (size_t iLight = 0; iLight < ShaderLightingDefines::MAX_NUM_POINT_LIGHTS; ++iLight) {
            m_point.lights[iLight].reset();
        }

        m_point.buffer->bind();
        PointLightsBuffer* pointLights = m_point.buffer->mapBuffer<PointLightsBuffer>();
        pointLights->numberOfLights = 0;
        m_point.buffer->unmapBuffer();
        m_point.buffer->unbind();

        m_point.count = 0;
    }
}

void Lighting::bindLightingBuffersToProgram(const std::shared_ptr<openrl::Program> program)
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

    // Point lights.
    {
        RLint blockIndex = program->getUniformBlockIndex("PointLights");
        if (blockIndex != -1) {
            program->setUniformBlock(blockIndex, m_point.buffer->buffer());
        }
    }
}

std::shared_ptr<EnvironmentLight> Lighting::addEnvironmentLight()
{
    m_environment.light = std::shared_ptr<EnvironmentLight>(new EnvironmentLight(m_environment.buffer));

    m_environment.light->primitive()->bind();
    m_environment.light->program()->bind();
    bindLightingBuffersToProgram(m_environment.light->program());
    m_environment.light->primitive()->unbind();

    updateLight(m_environment.light);

    if (m_lightCreatedCallback) {
        m_lightCreatedCallback(m_environment.light);
    }
    return m_environment.light;
}

void Lighting::removeEnvironmentLight()
{
    m_environment.light.reset();

    m_environment.buffer->bind();
    EnvironmentLightBuffer* environmentLight = m_environment.buffer->mapBuffer<EnvironmentLightBuffer>();
    environmentLight->primitive = RL_NULL_PRIMITIVE;
    environmentLight->texture = RL_NULL_TEXTURE;
    m_environment.buffer->unmapBuffer();
    m_environment.buffer->unbind();
}

void Lighting::updateLight(std::shared_ptr<EnvironmentLight> light)
{
    m_environment.buffer->bind();
    EnvironmentLightBuffer* environmentLight = m_environment.buffer->mapBuffer<EnvironmentLightBuffer>();
    light->copyToLightBuffer(environmentLight);
    m_environment.buffer->unmapBuffer();
    m_environment.buffer->unbind();
}

std::shared_ptr<DirectionalLight> Lighting::addDirectionalLight()
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

        if (m_lightCreatedCallback) {
            m_lightCreatedCallback(light);
        }
        return light;
    }

    LOG_ERROR("Attempting to add too many Directional Lights!");
    return nullptr;
}

void Lighting::updateLight(std::shared_ptr<DirectionalLight> light)
{
    m_directional.buffer->bind();
    DirectionalLightsBuffer* directionalLights = m_directional.buffer->mapBuffer<DirectionalLightsBuffer>();
    light->copyToLightBuffer(directionalLights);
    directionalLights->numberOfLights = m_directional.count;
    m_directional.buffer->unmapBuffer();
    m_directional.buffer->unbind();
}

void Lighting::removeLight(std::shared_ptr<DirectionalLight> light)
{
    // Find this light in our list of directional lights.
    size_t index = 0;
    for (; index < m_directional.count; ++index) {
        if (m_directional.lights[index] == light) {
            break;
        }
    }
    assert(index < m_directional.count);

    // If this is the last light in the list, just get rid of it.
    // Otherwise, swap this light with the last light in the list
    // in order to keep the light list compact.
    size_t finalIndex = m_directional.count - 1;
    if (index != finalIndex) {
        std::swap(m_directional.lights[index], m_directional.lights[finalIndex]);
        m_directional.lights[index]->updateLightIndex(index);
    }

    // Finally, remove the light.
    m_directional.lights[finalIndex].reset();
    --m_directional.count;

    {
        m_directional.buffer->bind();
        DirectionalLightsBuffer* directionalLights = m_directional.buffer->mapBuffer<DirectionalLightsBuffer>();
        directionalLights->primitives[finalIndex] = RL_NULL_PRIMITIVE;
        directionalLights->numberOfLights = m_directional.count;
        m_directional.buffer->unmapBuffer();
        m_directional.buffer->unbind();
    }
}

std::shared_ptr<PointLight> Lighting::addPointLight()
{
    if (m_point.count < ShaderLightingDefines::MAX_NUM_POINT_LIGHTS) {
        size_t lightIndex = m_point.count;

        std::shared_ptr<PointLight> light = std::shared_ptr<PointLight>(new PointLight(lightIndex, m_point.buffer));
        m_point.lights[lightIndex] = light;

        light->primitive()->bind();
        light->program()->bind();
        bindLightingBuffersToProgram(light->program());
        light->primitive()->unbind();

        ++m_point.count;

        // Update the lighting buffer.
        updateLight(light);

        if (m_lightCreatedCallback) {
            m_lightCreatedCallback(light);
        }
        return light;
    }

    LOG_ERROR("Attempting to add too many Point Lights!");
    return nullptr;
}

void Lighting::updateLight(std::shared_ptr<PointLight> light)
{
    m_point.buffer->bind();
    PointLightsBuffer* pointLights = m_point.buffer->mapBuffer<PointLightsBuffer>();
    light->copyToLightBuffer(pointLights);
    pointLights->numberOfLights = m_point.count;
    m_point.buffer->unmapBuffer();
    m_point.buffer->unbind();
}

void Lighting::removeLight(std::shared_ptr<PointLight> light)
{
    // Find this light in our list of point lights.
    size_t index = 0;
    for (; index < m_point.count; ++index) {
        if (m_point.lights[index] == light) {
            break;
        }
    }
    assert(index < m_point.count);

    // If this is the last light in the list, just get rid of it.
    // Otherwise, swap this light with the last light in the list
    // in order to keep the light list compact.
    size_t finalIndex = m_point.count - 1;
    if (index != finalIndex) {
        std::swap(m_point.lights[index], m_point.lights[finalIndex]);
        m_point.lights[index]->updateLightIndex(index);
    }

    // Finally, remove the light.
    m_point.lights[finalIndex].reset();
    --m_point.count;

    {
        m_point.buffer->bind();
        PointLightsBuffer* pointLights = m_point.buffer->mapBuffer<PointLightsBuffer>();
        pointLights->primitives[finalIndex] = RL_NULL_PRIMITIVE;
        pointLights->numberOfLights = m_point.count;
        m_point.buffer->unmapBuffer();
        m_point.buffer->unbind();
    }
}
