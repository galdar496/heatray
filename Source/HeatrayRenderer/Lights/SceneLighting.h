//
//  SceneLighting.h
//  Heatray
//
//  Utilities for defining the light buffers for a scene.
//

#pragma once

#include "ShaderLightingDefines.h"

#include <RLWrapper/Buffer.h>

#include <vector>

// Forward declarations.
class EnvironmentLight;
class DirectionalLight;
class PointLight;
namespace openrl {
class Buffer;
class Program;
} // namespace openrl.

class SceneLighting
{
public:
    SceneLighting();
    ~SceneLighting() = default;

    void clear();

    ///
    /// Binds the internal lighting buffers to an OpenRL program.
    ///
    void bindLightingBuffersToProgram(std::shared_ptr<openrl::Program> program);

    std::shared_ptr<DirectionalLight> addDirectionalLight();
    void updateLight(std::shared_ptr<DirectionalLight> light);
    void removeLight(std::shared_ptr<DirectionalLight> light);

    std::shared_ptr<PointLight> addPointLight();
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
        size_t count = 0;
    } m_directional;

    struct Point {
        std::shared_ptr<PointLight> lights[ShaderLightingDefines::MAX_NUM_POINT_LIGHTS] = { nullptr };
        std::shared_ptr<openrl::Buffer> buffer = nullptr;
        size_t count = 0;
    } m_point;
};
