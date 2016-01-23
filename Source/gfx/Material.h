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

#define MATERIAL_TYPES \
    X(DIFFUSE)         \
    X(SPECULAR)        \
    X(TRANSMISSIVE)    \
    X(DIFFUSE_TEXTURE) \
    X(NORMALMAP)       \
    X(LIGHT)           \
    X(SUBSURFACE)

/// Material properties supported by the renderer.
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
    
    /// Default constructor.
    Material();
    
    /// Destroy this material.
    void destroy();
    
    math::Vec3f diffuse;		// RGB values of the diffuse component for this material.
    math::Vec3f specular;		// RGB values of the specular component for this material.
    math::Vec3f transmissive;	// RGB values of the transmissive component for this material.
    
    float index_of_refraction;	// Index of refraction for this material.
    float roughness;            // Roughness of this material. 0 means perfectly smooth.
    
    gfx::Texture diffuse_texture; // Diffuse texture object to use for this material.
    gfx::Texture normal_texture;  // Normal texture map obect to use for this material.
    
    std::string name;           // Name of this material.
    
    std::bitset<NUM_COMPONENT_FLAGS> component_flags; // Type of components defined in this material.

    // Table of the material names.
    static const std::string material_names[];
};

} // namespace gfx