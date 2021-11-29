//
//  EnvironmentLight.h
//  Heatray
//
//  Handles loading of IBL data and the creation of any necessary OpenRL
//  objects to utilize environment lighting.
//

#pragma once

#include "Light.h"
#include "ShaderLightingDefines.h"

#include <RLWrapper/Primitive.h>
#include <Utility/TextureLoader.h>

#include <string>

// Forward declarations.
namespace openrl {
class Buffer;
class Program;
class Texture;
} // namespace openrl.

class EnvironmentLight final : public Light
{
public:
    explicit EnvironmentLight(std::shared_ptr<openrl::Buffer> lightBuffer);
    ~EnvironmentLight() = default;

    //-------------------------------------------------------------------------
    // Alter the source of the environment light. Any primitive bindings
    // etc will still be valid after this function executes.
    void changeImageSource(const char* path, bool builtInMap);

    //-------------------------------------------------------------------------
    // Switches the internal image data to be a constant bright grey
    // in order to enable a white furnace test for BRDF testing.
    void enableWhiteFurnaceTest();

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
};
