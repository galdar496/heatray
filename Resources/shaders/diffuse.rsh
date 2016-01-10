//
//  diffuse.rsh
//  Heatray
//
//  Created by Cody White on 5/10/14.
//
//
// Implement diffuse lighting and shadowing.

uniform vec3 kd; // Diffuse color.
uniform int subsurface;
uniform primitive subsurface_primitive;

void diffuse()
{
    vec3 base_color     = rl_InRay.color * kd;
    vec3 surface_normal = getNormal();

    for (int ii = 0; ii < Light.count; ++ii)
    {
        vec3 light_dir = Light.positions[ii] - rl_IntersectionPoint;
        float vec_length = length(light_dir);
        light_dir /= vec_length; // normalize.
        
        float ndotl = dot(light_dir, surface_normal);
        
        if (ndotl > 0.0)
        {
            // Determine the angle between the direction of the light and the light's direction.
            float angle = dot(Light.normals[ii], -light_dir);
            
            if (angle > 0.0)
            {
				// Fire a shadow ray at the light source.
                createRay();
				rl_OutRay.direction = light_dir;
				rl_OutRay.color = base_color * angle * ndotl;
                rl_OutRay.maxT = vec_length;
                rl_OutRay.defaultPrimitive = Light.lights[ii];
                rl_OutRay.occlusionTest = true;
                emitRay();
            }
        }
//        else if (subsurface != 0)
//        {   
//            createRay();
//            rl_OutRay.direction = light_dir;
//            rl_OutRay.color = base_color;
//            rl_OutRay.prefixPrimitive = subsurface_primitive;
//            emitRay();
//        }
    }
    
    if (GI.enabled != 0)
    {
        diffuseBounce(base_color, surface_normal);
    }
}




