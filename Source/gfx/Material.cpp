//
//  Material.cpp
//  Heatray
//
//  Created by Cody White on 1/11/64.
//
//

#include "Material.h"

namespace gfx
{

#define X(type) "MATERIAL_" + std::string(#type),
    const std::string Material::material_names[] =
    {
        MATERIAL_TYPES
    };

#undef X

Material::Material() :
    index_of_refraction(0.0f),
    roughness(0.0f),
    name("<unnamed>"),
    component_flags(0)
{
}

/// Destroy this material.
void Material::destroy()
{
    diffuse_texture.destroy();
    normal_texture.destroy();
}

} // namespace gfx