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
    const std::string Material::materialNames[] =
    {
        MATERIAL_TYPES
    };

#undef X

Material::Material() :
    indexOfRefraction(0.0f),
    roughness(0.0f),
    name("<unnamed>"),
    componentFlags(0)
{
}

void Material::Destroy()
{
    diffuseTexture.Destroy();
    normalTexture.Destroy();
}

} // namespace gfx