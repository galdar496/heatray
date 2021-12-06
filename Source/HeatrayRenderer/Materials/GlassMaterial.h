//
//  GlassMaterial.h
//  Heatray
//
//  Microfacet refractive material suitable for modeling glass/transparent objects.
//

#pragma once

#include "Material.h"

#include <glm/glm/vec3.hpp>

class GlassMaterial final : public Material
{
public:
    GlassMaterial(const std::string &name)
        : Material(name, Material::Type::Glass) {}
    virtual ~GlassMaterial() = default;

    struct Parameters {
        std::shared_ptr<openrl::Texture> baseColorTexture = nullptr;
        std::shared_ptr<openrl::Texture> normalmap = nullptr;
        std::shared_ptr<openrl::Texture> metallicRoughnessTexture = nullptr;
        glm::vec3 baseColor = glm::vec3(1.0f);  // Color applied to object (linear). If the object is a dialectric, this color represents albedo, otherwise this is the specular reflection color.   
        float roughness = 1.0f;                 // [0-1] such that 0 == completely smooth and 1 == completely rough.
        float ior = 1.57f;                      // [0-infinity]. Index of refraction (IOR) for this material. Default to glass.
        float density = 0.05f;                  // Density [0-1] which controls how much light gets absorbed as it moves through this surface.

        bool forceEnableAllTextures = false;    // If true, all shader texture options are enabled even if no texture is present.
    };

    void build() override;
    void rebuild() override;
    void modify() override;

    Parameters& parameters() { return m_params; }

private:
    static constexpr char const* m_shader = "glass.rlsl"; // Shader file with corresponding material code.
    std::shared_ptr<openrl::Texture> m_dummyTexture = nullptr;

    Parameters m_params;
};
