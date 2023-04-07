//
//  EnvironmentLight.h
//  Heatray
//
//  Handles loading of IBL data and the creation of any necessary OpenRL
//  objects to utilize environment lighting.
//

#pragma once

#if 0

#include "Light.h"
#include "ShaderLightingDefines.h"

#include <RLWrapper/Primitive.h>
#include <Utility/TextureLoader.h>

#include <glm/glm/vec3.hpp>

#include <string>
#include <string_view>

// Forward declarations.
namespace openrl {
class Buffer;
class Program;
class Texture;
} // namespace openrl.

class EnvironmentLight final : public Light
{
public:
    explicit EnvironmentLight(const std::string_view name, std::shared_ptr<openrl::Buffer> lightBuffer);
    ~EnvironmentLight() = default;

    //-------------------------------------------------------------------------
    // Alter the source of the environment light. Any primitive bindings
    // etc will still be valid after this function executes.
    void changeImageSource(const std::string_view path, bool builtInMap);

    //-------------------------------------------------------------------------
    // Switches the internal image data to be a constant color.
    static constexpr std::string_view SOLID_COLOR = "solid color";
    void enableSolidColor(const glm::vec3 &color);

    //-------------------------------------------------------------------------
    // Apply a rotation to the IBL (theta-only).
    void rotate(const float theta_radians);

    //-------------------------------------------------------------------------
    // Adjust the exposure of the IBL.
    void setExposure(const float exposureCompensation);

    //-------------------------------------------------------------------------
    // Copy to the light buffer that represents the environment light
    // in a scene.
    void copyToLightBuffer(EnvironmentLightBuffer* buffer);

private:
    std::shared_ptr<openrl::Texture> m_texture = nullptr;

    float m_exposureCompensation = 0.0f;
    float m_thetaRotation = 0.0f;

    std::string m_textureSourcePath;

    glm::vec3 m_solidColor = glm::vec3(0.0f);
};

#endif
