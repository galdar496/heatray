//
//  DirectionalLight.h
//  Heatray
//
//  Defines a single directional light along with any OpenRL
//  resources that are required.
//

#pragma once

#include "ShaderLightingDefines.h"

#include <RLWrapper/Buffer.h>
#include <RLWrapper/Primitive.h>

#include <glm/glm/glm.hpp>

// Forward declarations.
namespace openrl {
class Program;
} // namespace openrl.

class DirectionalLight
{
public:
    explicit DirectionalLight(size_t lightIndex, std::shared_ptr<openrl::Buffer> lightBuffer);
    ~DirectionalLight() = default;

    struct Params {
        glm::vec3 color;
        float intensity = 1.0f;

        // Orientation controls for the directional light.
        struct Orientation {
            float phi = 0.0f; // In radians [0 - 2π]
            float theta = 0.0f; // In radians [-π/2 - π/2]
        } orientation;
    };

    ///
    /// Copy to the light buffer that represents all directional lights
    /// in a scene. It is assumed that this light will copy into the 
    /// right part of the buffer.
    ///
    void copyToLightBuffer(DirectionalLightsBuffer* buffer);

    Params params() const { return m_params; }
    void setParams(const Params &other);
    
    std::shared_ptr<openrl::Program> program() const { return m_program; }
    std::shared_ptr<openrl::Primitive> primitive() const { return m_primitive; }
private:
    void calculateOrientation();
    void setUniforms() const;

    std::shared_ptr<openrl::Primitive> m_primitive = nullptr;
    std::shared_ptr<openrl::Program> m_program = nullptr;

    Params m_params;
    glm::vec3 m_direction;

    // This represents the index of this light within the global buffer
    // of all directional lights.
    const size_t m_lightIndex = 0;  
};
