//
//  PointLight.h
//  Heatray
//
//  Defines a single point light along with any OpenRL
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

class PointLight
{
public:
    explicit PointLight(size_t lightIndex, std::shared_ptr<openrl::Buffer> lightBuffer);
    ~PointLight() = default;

    //-------------------------------------------------------------------------
    // Parameters used to define and control a directional light.
    struct Params {
        glm::vec3 color = glm::vec3(1.0f);
        glm::vec3 position = glm::vec3(0.0f);
        float intensity = 1.0f;
    };

    //-------------------------------------------------------------------------
    // Copy to the light buffer that represents all point lights
    // in a scene. It is assumed that this light will copy into the 
    // right part of the buffer.
    void copyToLightBuffer(PointLightsBuffer* buffer);

    Params params() const { return m_params; }
    void setParams(const Params &params) { m_params = params; }
    
    std::shared_ptr<openrl::Program> program() const { return m_program; }
    std::shared_ptr<openrl::Primitive> primitive() const { return m_primitive; }

    //-------------------------------------------------------------------------
    // Update the light index for this point light. This can happen when
    // lights are deleted and the lighting buffer needs to reshuffle lights to
    // keep the buffer tightly packed.
    void updateLightIndex(const size_t newLightIndex);
private:
    void setUniforms() const;

    std::shared_ptr<openrl::Primitive> m_primitive = nullptr;
    std::shared_ptr<openrl::Program> m_program = nullptr;

    Params m_params;

    // This represents the index of this light within the global buffer
    // of all directional lights.
    size_t m_lightIndex = 0;  
};
