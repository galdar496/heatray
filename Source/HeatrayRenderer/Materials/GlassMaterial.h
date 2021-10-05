//
//  GlassMaterial.h
//  Heatray
//
//  Microfacet refractive material suitable for modeling glass/transparent objects.
//

#pragma once

#include "Material.h"

#include <glm/glm/vec3.hpp>

class GlassMaterial : public Material
{
public:
    GlassMaterial() = default;
    virtual ~GlassMaterial() = default;

    struct Parameters {
        glm::vec3 baseColor = glm::vec3(1.0f);  ///< Color applied to object (linear). If the object is a dialectric, this color represents albedo, otherwise this is the specular reflection color.   
        float roughness = 1.0f;                 ///< [0-1] such that 0 == completely smooth and 1 == completely rough.
        float ior = 1.57f;                      ///< [0-infinity]. Index of refraction (IOR) for this material. Default to glass.
        float density = 0.05f;                  ///< Density [0-1] which controls how much light gets absorbed as it moves through this surface.
    };

	void build() override;
	void rebuild() override;
	void modify() override;

	Parameters& parameters() { return m_params; }

private:
    static constexpr char const* m_shader = "glass.rlsl"; ///< Shader file with corresponding material code.

	Parameters m_params;
};
