//
//  passthrough.vert
//  Heatray
//
//  Created by Cody White on 5/10/14.
//
//

attribute vec3 position_attribute;
attribute vec3 normal_attribute;
attribute vec2 tex_coord_attribute;
attribute vec3 tangent_attribute;

varying vec3 normal;
varying vec2 tex_coords;
varying vec3 tangent;
varying vec3 bitangent;

void main()
{
    rl_Position = vec4(position_attribute, 1.0);
    
    // Varyings.
    normal      = normal_attribute;
    tex_coords  = tex_coord_attribute;
    tangent     = tangent_attribute;
    bitangent   = cross(normal, tangent);
}