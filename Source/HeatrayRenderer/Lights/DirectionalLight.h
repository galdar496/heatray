//
//  DirectionalLight.h
//  Heatray
//
//  Defines a single directional light along with any OpenRL
//  resources that are required.
//

#pragma once

#include "Light.h"
#include "ShaderLightingDefines.h"

#include <RLWrapper/Buffer.h>
#include <RLWrapper/Primitive.h>

#include <glm/glm/glm.hpp>

// Forward declarations.
namespace openrl {
class Program;
} // namespace openrl.

class DirectionalLight final : public Light
{
public:
    explicit DirectionalLight(const std::string& name, size_t lightIndex, std::shared_ptr<openrl::Buffer> lightBuffer);
    ~DirectionalLight() = default;

    //-------------------------------------------------------------------------
    // Parameters used to define and control a directional light.
    struct Params {
        glm::vec3 color = glm::vec3(1.0f);
        float illuminance = 1.0f;

        // Orientation controls for the directional light.
        struct Orientation {
            float phi = 0.0f; // In radians [0 - 2π]
            float theta = 0.0f; // In radians [-π/2 - π/2]
        } orientation;
    };

    //-------------------------------------------------------------------------
    // Copy to the light buffer that represents all directional lights
    // in a scene. It is assumed that this light will copy into the 
    // right part of the buffer.
    void copyToLightBuffer(DirectionalLightsBuffer* buffer);

    Params params() const { return m_params; }
    void setParams(const Params &params);   

    //-------------------------------------------------------------------------
    // Update the light index for this directional light. This can happen when
    // lights are deleted and the lighting buffer needs to reshuffle lights to
    // keep the buffer tightly packed.
    void updateLightIndex(const size_t newLightIndex);
private:
    void calculateOrientation();
    void setUniforms() const;

    Params m_params;
    glm::vec3 m_direction = glm::vec3(0.0, 1.0, 0.0);

    // This represents the index of this light within the global buffer
    // of all directional lights.
    size_t m_lightIndex = 0;  
};
