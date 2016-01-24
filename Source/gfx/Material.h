//
//  Material.h
//  Heatray
//
//  Created by Cody White on 5/12/14.
//
//

#pragma once

#include "../math/Vector.h"
#include "Texture.h"
#include <string>
#include <bitset>

namespace gfx
{

// Add any future material types to this define and they will be properly
// propagated through the code.
#define MATERIAL_TYPES \
    X(DIFFUSE)         \
    X(SPECULAR)        \
    X(TRANSMISSIVE)    \
    X(DIFFUSE_TEXTURE) \
    X(NORMALMAP)       \
    X(LIGHT)           \
    X(SUBSURFACE)

///
/// Material properties supported by the renderer.
///

struct Material
{
    // What type of components are contained within this material?
#define X(type) type,
    enum MaterialComponents
    {
        MATERIAL_TYPES

        NUM_COMPONENT_FLAGS
    };
#undef X
    
    Material();
    
    ///
    /// Destroy this material. This function deallocates any OpenRL data
    /// that this day may be holding onto, e.g. textures.
    ///
    void Destroy();
    
    math::Vec3f diffuse;		///< RGB values of the diffuse component for this material.
    math::Vec3f specular;		///< RGB values of the specular component for this material.
    math::Vec3f transmissive;	///< RGB values of the transmissive component for this material.
    
    float indexOfRefraction;	///< Index of refraction for this material.
    float roughness;            ///< Roughness of this material. 0 means perfectly smooth.
    
    gfx::Texture diffuseTexture; ///< Diffuse texture object to use for this material.
    gfx::Texture normalTexture;  ///< Normal texture map obect to use for this material.
    
    std::string name;           ///< Name of this material.
    
    std::bitset<NUM_COMPONENT_FLAGS> componentFlags; ///< Type of components defined in this material.

    // Table of the material names.
    static const std::string materialNames[];
};

} // namespace gfx