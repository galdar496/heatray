//
//  PhysicallyBasedMaterial.h
//  Heatray
//
//  Microfacet material suitable for modeling hard-surfaces (non-transparent and non-transmissive).
//

#pragma once

#include "Material.h"

#include <glm/glm/vec3.hpp>
#include <memory>

class PhysicallyBasedMaterial final : public Material
{
public:
    explicit PhysicallyBasedMaterial(const std::string& name)
        : Material(name, Material::Type::PBR) {}
    virtual ~PhysicallyBasedMaterial() = default;

    struct Parameters {
        std::shared_ptr<openrl::Texture> baseColorTexture = nullptr;
        std::shared_ptr<openrl::Texture> emissiveTexture = nullptr;
        std::shared_ptr<openrl::Texture> normalmap = nullptr;
        std::shared_ptr<openrl::Texture> metallicRoughnessTexture = nullptr;
        std::shared_ptr<openrl::Texture> clearCoatTexture = nullptr;
        std::shared_ptr<openrl::Texture> clearCoatRoughnessTexture = nullptr;
        std::shared_ptr<openrl::Texture> clearCoatNormalmap = nullptr;
        glm::vec3 baseColor = glm::vec3(1.0f);      // Color applied to object (linear). If the object is a dialectric, this color represents albedo, otherwise this is the specular reflection color.
        glm::vec3 emissiveColor = glm::vec3(0.0f);  // Color applied to the emissive channel (linear).
        float roughness = 1.0f;                     // [0-1] such that 0 == completely smooth and 1 == completely rough.
        float metallic  = 0.0f;                     // [0-1] such that 0 == dialectric and 1 == conductor.
        float specularF0 = 0.5f;                    // [0-1] Specular value at Fresnel of 0 degrees.
        float clearCoat = 0.0f;                     // [0-1] Controls the clear coat scaling power. 
        float clearCoatRoughness = 0.0f;            // [0-1] such that 0 == completely smooth and 1 == completely rough.

        bool forceEnableAllTextures = false;        // If true, all shader texture options are enabled even if no texture is present.
    };

    void build() override;
    void rebuild() override;
    void modify() override;

    Parameters& parameters() { return m_params;  }

private:
    static constexpr char const * m_shader = "physicallyBased.rlsl"; // Shader file with corresponding material code.
    std::shared_ptr<openrl::Texture> m_multiscatterLUT = nullptr;
    std::shared_ptr<openrl::Texture> m_dummyTexture = nullptr;

    Parameters m_params;
};
