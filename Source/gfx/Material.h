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

/// Material properties supported by the renderer.
struct Material
{
    // What type of components are contained within this material?
    enum MaterialComponents
    {
        DIFFUSE = 0,
        SPECULAR,
        TRANSMISSIVE,
        SUBSURFACE,
    };
    
    /// Default constructor.
    Material() :
    	index_of_refraction(0.0f),
    	roughness(0.0f),
    	name("<unnamed>"),
    	component_flags(0)
    {
    }
    
    /// Destroy this material.
    void destroy()
    {
        diffuse_texture.destroy();
        normal_texture.destroy();
    }
    
    math::vec3f diffuse;		// RGB values of the diffuse component for this material.
    math::vec3f specular;		// RGB values of the specular component for this material.
    math::vec3f transmissive;	// RGB values of the transmissive component for this material.
    
    float index_of_refraction;	// Index of refraction for this material.
    float roughness;            // Roughness of this material. 0 means perfectly smooth.
    
    gfx::Texture diffuse_texture; // Diffuse texture object to use for this material.
    gfx::Texture normal_texture;  // Normal texture map obect to use for this material.
    
    std::string name;           // Name of this material.
    
    std::bitset<4> component_flags; // Type of components defined in this material.
};

} // namespace gfx