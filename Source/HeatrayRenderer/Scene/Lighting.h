//
//  Lighting.h
//  Heatray
//
//  Utilities for defining the light buffers for a scene.
//

#pragma once

#if 0

#include <HeatrayRenderer/Lights/Light.h>
#include <HeatrayRenderer/Lights/ShaderLightingDefines.h>

#include <functional>
#include <vector>

// Forward declarations.
class EnvironmentLight;
class DirectionalLight;
class PointLight;
class SpotLight;
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

    //-------------------------------------------------------------------------
    // Update a light, which re-uploads its parameters to OpenRL.
    void updateLight(std::shared_ptr<Light> light);

    //-------------------------------------------------------------------------
    // Remove a light, deleting its resources from OpenRL as well.
    void removeLight(std::shared_ptr<Light> light);

    std::shared_ptr<DirectionalLight> addDirectionalLight(const std::string_view name);
    const std::shared_ptr<DirectionalLight>* directionalLights() const { return &(m_directional.lights[0]); }
    void updateDirectionalLight(std::shared_ptr<DirectionalLight> light);
    void removeDirectionalLight(std::shared_ptr<DirectionalLight> light);

    std::shared_ptr<PointLight> addPointLight(const std::string_view name);
    const std::shared_ptr<PointLight>* pointLights() const { return &(m_point.lights[0]); }
    void updatePointLight(std::shared_ptr<PointLight> light);
    void removePointLight(std::shared_ptr<PointLight> light);

    std::shared_ptr<SpotLight> addSpotLight(const std::string_view name);
    const std::shared_ptr<SpotLight>* spotLights() const { return &(m_spot.lights[0]); }
    void updateSpotLight(std::shared_ptr<SpotLight> light);
    void removeSpotLight(std::shared_ptr<SpotLight> light);

    std::shared_ptr<EnvironmentLight> addEnvironmentLight();
    void removeEnvironmentLight();
    void updateEnvironmentLight(std::shared_ptr<EnvironmentLight> light);

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

    struct Spot {
        std::shared_ptr<SpotLight> lights[ShaderLightingDefines::MAX_NUM_SPOT_LIGHTS] = { nullptr };
        std::shared_ptr<openrl::Buffer> buffer = nullptr;
        int count = 0;
    } m_spot;

    LightCreatedCallback m_lightCreatedCallback;
};

#endif
