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

class PhysicallyBasedMaterial : public Material
{
public:
    PhysicallyBasedMaterial() = default;
    virtual ~PhysicallyBasedMaterial() = default;

    struct Parameters
    {
        glm::vec3 baseColor = glm::vec3(1.0f);  ///< Color applied to object (linear). If the object is a dialectric, this color represents albedo, otherwise this is the specular reflection color.   
        std::shared_ptr<openrl::Texture> baseColorTexture;
        std::shared_ptr<openrl::Texture> metallicRoughnessTexture;
        float roughness = 1.0f;                 ///< [0-1] such that 0 == completely smooth and 1 == completely rough.
        float metallic  = 0.0f;                 ///< [0-1] such that 0 == dialectric and 1 == conductor.
        float specularF0 = 0.5f;                ///< [0-1]. Specular value at Fresnel of 0 degrees.
    };

    void build(const Parameters& params);

private:
    static constexpr char const * m_shader = "physicallyBased.rlsl"; ///< Shader file with corresponding material code.
};
