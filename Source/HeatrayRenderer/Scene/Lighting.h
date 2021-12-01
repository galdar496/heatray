﻿//
//  Lighting.h
//  Heatray
//
//  Utilities for defining the light buffers for a scene.
//

#pragma once

#include <HeatrayRenderer/Lights/Light.h>
#include <HeatrayRenderer/Lights/ShaderLightingDefines.h>

#include <functional>
#include <vector>

// Forward declarations.
class EnvironmentLight;
class DirectionalLight;
class PointLight;
namespace openrl {
class Buffer;
class Program;
} // namespace openrl.

class Lighting
{
public:
    Lighting();
    ~Lighting() = default;

    //-------------------------------------------------------------------------
    // Get rid of all lights in the scene. Note that this function does not
    // destroy any buffers etc, just clears them.
    void clear();

    //-------------------------------------------------------------------------
    // Get rid of all lights in the scene that aren't environment lights. Note 
    // that this function does not destroy any buffers etc, just clears them.
    void clearAllButEnvironment();

    //-------------------------------------------------------------------------
    // Binds the internal lighting buffers to an OpenRL program.
    void bindLightingBuffersToProgram(const std::shared_ptr<openrl::Program> program);

    //-------------------------------------------------------------------------
    // Install a callback that is always invoked whenever a new light is created.
    using LightCreatedCallback = std::function<void(std::shared_ptr<Light> light)>;
    void installLightCreatedCallback(LightCreatedCallback &&callback) { m_lightCreatedCallback = std::move(callback); }

    std::shared_ptr<DirectionalLight> addDirectionalLight(const std::string &name);
    const std::shared_ptr<DirectionalLight>* directionalLights() const { return &(m_directional.lights[0]); }
    void updateLight(std::shared_ptr<DirectionalLight> light);
    void removeLight(std::shared_ptr<DirectionalLight> light);

    std::shared_ptr<PointLight> addPointLight(const std::string& name);
    const std::shared_ptr<PointLight>* pointLights() const { return &(m_point.lights[0]); }
    void updateLight(std::shared_ptr<PointLight> light);
    void removeLight(std::shared_ptr<PointLight> light);

    std::shared_ptr<EnvironmentLight> addEnvironmentLight();
    void removeEnvironmentLight();
    void updateLight(std::shared_ptr<EnvironmentLight> light);

private:
    struct Environment {
        std::shared_ptr<EnvironmentLight> light = nullptr;
        std::shared_ptr<openrl::Buffer> buffer = nullptr;
    } m_environment;

    struct Directional {
        std::shared_ptr<DirectionalLight> lights[ShaderLightingDefines::MAX_NUM_DIRECTIONAL_LIGHTS] = { nullptr };
        std::shared_ptr<openrl::Buffer> buffer = nullptr;
        int count = 0;
    } m_directional;

    struct Point {
        std::shared_ptr<PointLight> lights[ShaderLightingDefines::MAX_NUM_POINT_LIGHTS] = { nullptr };
        std::shared_ptr<openrl::Buffer> buffer = nullptr;
        int count = 0;
    } m_point;

    LightCreatedCallback m_lightCreatedCallback;
};