//
//  refraction.rsh
//  Heatray
//
//  Created by Cody White on 5/10/14.
//
//
// Implement refraction using fresnel.

// First 3 components are the refractive color per channel, the last is the IOR for this material.
uniform vec4 kt;

void transmissive()
{
    // Calculate the fresnel coefficients.
    float ref;
    vec3 actual_normal;
    if (rl_FrontFacing)
    {
        ref = 1.0 / kt.w;
        actual_normal = normalize(normal);
    }
    else
    {
        ref = kt.w;
        actual_normal = normalize(-normal);
    }
    
    float cos_theta_i = abs(dot(actual_normal, -1.0 * rl_InRay.direction));
    float sin_theta_t_2 = (ref * ref) * (1.0 - (cos_theta_i * cos_theta_i));
    float reflectance = 1.0;
    float refraction = 0.0;
    float cos_theta_t;
    if (sin_theta_t_2 < 1.0)
    {
        cos_theta_t = sqrt(1.0 - sin_theta_t_2);
        float reflectance_paral = (ref * cos_theta_i - cos_theta_t) / (ref * cos_theta_i + cos_theta_t);
        float reflectance_ortho = (cos_theta_i - ref * cos_theta_t) / (cos_theta_i + ref * cos_theta_t);
        reflectance_ortho *= reflectance_ortho;
        reflectance_paral *= reflectance_paral;
        reflectance = 0.5 * (reflectance_ortho + reflectance_paral);
        refraction = 1.0 - reflectance;
    }
    
    if (reflectance > 0.0)
    {
        specular(rl_InRay.color * kt.xyz * reflectance, actual_normal);
    }
    
    if (refraction > 0.0)
    {
        vec3 refracted_dir = (ref * rl_InRay.direction) + (actual_normal * ((ref * cos_theta_i) - cos_theta_t));

        createRay();
        rl_OutRay.direction = refracted_dir;
        rl_OutRay.color = rl_InRay.color * kt.xyz * refraction;
        if (rl_InRay.is_diffuse_bounce)
        {
            rl_OutRay.color *= vec3(kt.w); // Fake caustics.
        }
        emitRay();
    }
}


