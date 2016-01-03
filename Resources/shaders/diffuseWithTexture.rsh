//
//  diffuseWithTexture.rsh
//  Heatray
//
//  Created by Cody White on 5/10/14.
//
//
// Implement diffuse lighting and shadowing using a diffuse texture.

uniform vec3 kd;                   // Diffuse color.
uniform sampler2D diffuse_texture; // Diffuse texture to use for shading.

void diffuse()
{
    vec3 base_color = rl_InRay.color * kd * texture2D(diffuse_texture, tex_coords).zyx;
    vec3 normalized_normal = getNormal();
    
    for (int ii = 0; ii < Light.count; ++ii)
    {
        vec3 light_dir = Light.positions[ii] - rl_IntersectionPoint;
        float vec_length = length(light_dir);
        light_dir /= vec_length; // normalize.
        
        float ndotl = dot(light_dir, normalized_normal);
        
        if (ndotl > 0.0)
        {
            // Determine the angle between the direction of the light and the light's direction.
            float angle = dot(Light.normals[ii], -light_dir);
            
            if (angle > 0.0)
            {
                createRay();
                rl_OutRay.direction = light_dir;
                rl_OutRay.color = base_color * angle * ndotl;
                rl_OutRay.maxT = vec_length - 0.15; // Shorten max length a bit to avoid precision issues.
                rl_OutRay.defaultPrimitive = Light.lights[ii];
                rl_OutRay.occlusionTest = true;
                emitRay();
            }
        }
    }
    
    if (GI.enabled != 0)
    {
        diffuseBounce(base_color, normalized_normal);
    }
}




