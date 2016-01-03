//
//  specular.rsh
//  Heatray
//
//  Created by Cody White on 5/10/14.
//
//
// Implement a mirror reflection.


// First 3 components are the specular reflection powers per color channel. The last is the roughness.
uniform vec4 ks;

void specular(vec3 ray_power, vec3 normalized_normal)
{
    vec3 offset = vec3(0.0);
    
    // If this surface has any roughness applied to it create an offset for the ray
    // using perlin noise. I know this isn't correct glossy reflection, but it works
    // visually for now.
    if (ks.w > 0.0)
    {
        offset = noise3(Light.positions[0]) * ks.w;
    }
    
    createRay();
    rl_OutRay.direction = reflect(rl_InRay.direction, normalized_normal) + offset;
    rl_OutRay.color = ray_power;
    emitRay();
}


