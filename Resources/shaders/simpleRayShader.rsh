//
//  simpleRayShader.rsh
//  Heatray
//
//  Created by Cody White on 1/11/16.
//
//
// Implement a simple pathtracing pipeline as an uber shader.
// This shader utilizes several defines that are specified by Heatray to enable/disable different features of this shader.
// Those defines are:
/*
	MATERIAL_DIFFUSE
	MATERIAL_SPECULAR
	MATERIAL_TRANSMISSIVE
	MATERIAL_DIFFUSE_TEXTURE
	MATERIAL_NORMALMAP
	MATERIAL_SUBSURFACE
*/
// Place your material-specific code within the proper defines in order to streamline a running shader to be material-specific.

rayattribute vec3 color; // Color value to use for the final ray accumulation.
rayattribute bool is_diffuse_bounce; // If true, this ray is a diffuse bounce used for path tracing.

varying vec3 normal;
varying vec2 tex_coords;

// Uniform values.
uniformblock Light
{
	// MAX_LIGHTS is defined by the Heatray source code.
	
	vec3  position[MAX_LIGHTS]; // Position of this light in world-space.
	vec3  normal[MAX_LIGHTS];    // Direction this light is shining.
	
	int count; // How many lights are currently active.
	primitive light_primitive[MAX_LIGHTS]; // The RL primitive for the lights.
};

uniform primitive defaultPrim; // Default primitive to use if the ray misses any geometric primitives.

#ifdef MATERIAL_DIFFUSE
	uniform vec3 kd; // Diffuse color.
	uniform float bounce_probablility; // This is the probability that this ray will make a diffuse bounce (kd.x + kd.y + kd.z) / 3.
	
	// Uniform block which contains a texture full of random values between 0 and 1.
	// These are used to determine a new sample in the hemisphere for GI rays.
	uniformblock GI
	{
		sampler2D random_values; // Texture full of random values.
		int enabled;             // Flag to turn on/off GI (path tracing).
	};
	
	varying vec3 tangent;
	varying vec3 bitangent;
	
	#ifdef MATERIAL_DIFFUSE_TEXTURE
		uniform sampler2D diffuse_texture; // Diffuse texture to use for shading.
	#endif
	
	#ifdef MATERIAL_NORMALMAP
		uniform sampler2D normalmap;
	#endif
	
#endif

#if defined(MATERIAL_SPECULAR) || defined(MATERIAL_TRANSMISSIVE)
	// First 3 components are the specular reflection powers per color channel. The last is the roughness.
	uniform vec4 ks;
#endif

#ifdef MATERIAL_TRANSMISSIVE
	// First 3 components are the refractive color per channel, the last is the IOR for this material.
	uniform vec4 kt;
#endif

#ifdef MATERIAL_SUBSURFACE
	uniform int subsurface;
	uniform primitive subsurface_primitive;
#endif

// OpenRL setup shader to specify how many rays at most this shader will emit.
void setup()
{
	// Determine the number of rays that will be emitted by this material.
	int num_rays_to_emit = 0;
	
	#ifdef MATERIAL_DIFFUSE
		// Diffuse bounce + shadow rays.
		num_rays_to_emit += 1 + Light.count;
	#endif

	#ifdef MATERIAL_SPECULAR
		// Reflection ray.
		num_rays_to_emit += 1;
	#endif

	#ifdef MATERIAL_TRANSMISSIVE
		// Reflection and refraction ray.
		num_rays_to_emit += 2;
	#endif

	#ifdef MATERIAL_SUBSURFACE
		// Subsurface ray.
		num_rays_to_emit += 1;
	#endif

	rl_OutputRayCount[0] = num_rays_to_emit;
}

#ifdef MATERIAL_DIFFUSE
vec3 getNormal()
{
	#ifdef MATERIAL_NORMALMAP
		// Convert the tangent-space normal into world-space.
		vec3 norm = texture2D(normalmap, tex_coords).zyx * 2.0 - 1.0;
		return mat3(normalize(tangent), normalize(bitangent), normalize(normal)) * norm;
	#else
		return normalize(normal);
	#endif
}

// Generate a random sample in the hemisphere using cosine-weighted sampling. This random
// sample is represented in tangent space.
vec3 cosineSampleHemisphere(vec4 rand)
{
    float r = sqrt(rand.x);
    float theta = 2.0 * 3.14159 * rand.y;
    float x = r * cos(theta);
    float y = r * sin(theta);
    return vec3(x, y, sqrt(max(0.0, 1.0 - rand.x)));
}

// Generate a diffuse bounce ray and emit it.
void diffuseBounce(vec3 base_color, vec3 normalized_normal)
{
    vec4 rand = texture2D(GI.random_values, noise2(Light.position[0].xy * rl_FrameCoord.xy));
    
    // Russian roulette.
    if (rand.z <= bounce_probablility)
    {
        // Construct a basis matrix out of the tangent, bitangent, and normal. This matrix is
        // used to transform the random hemisphere sample into world space.
        mat3 basis = mat3(normalize(tangent), normalize(bitangent), normalized_normal);
        
        vec3 sample_point = cosineSampleHemisphere(rand);
        
        createRay();
        rl_OutRay.direction         = basis * sample_point;
        rl_OutRay.color             = base_color;
        rl_OutRay.is_diffuse_bounce = true;
        emitRay();
    }
}

void diffuse()
{
    vec3 base_color     = rl_InRay.color * kd;
    vec3 surface_normal = getNormal();
	
	#ifdef MATERIAL_DIFFUSE_TEXTURE
		base_color *= texture2D(diffuse_texture, tex_coords).zyx;
	#endif

    for (int ii = 0; ii < Light.count; ++ii)
    {
        vec3 light_dir = Light.position[ii] - rl_IntersectionPoint;
        float vec_length = length(light_dir);
        light_dir /= vec_length; // normalize.
        
        float ndotl = dot(light_dir, surface_normal);
        
        if (ndotl > 0.0)
        {
            // Determine the angle between the direction of the light and the light's direction.
            float angle = dot(Light.normal[ii], -light_dir);
            
            if (angle > 0.0)
            {
				// Fire a shadow ray at the light source.
                createRay();
				rl_OutRay.direction = light_dir;
				rl_OutRay.color = base_color * angle * ndotl;
                rl_OutRay.maxT = vec_length;
                rl_OutRay.defaultPrimitive = Light.light_primitive[ii];
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
#endif

#if defined(MATERIAL_SPECULAR) || defined(MATERIAL_TRANSMISSIVE)
void specular(vec3 ray_power, vec3 normalized_normal)
{
    vec3 offset = vec3(0.0);
    
    // If this surface has any roughness applied to it create an offset for the ray
    // using perlin noise. I know this isn't correct glossy reflection, but it works
    // visually for now.
    if (ks.w > 0.0)
    {
        offset = noise3(Light.position[0]) * ks.w;
    }
    
    createRay();
    rl_OutRay.direction = reflect(rl_InRay.direction, normalized_normal) + offset;
    rl_OutRay.color = ray_power;
    emitRay();
}
#endif

#ifdef MATERIAL_TRANSMISSIVE
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
#endif

void main()
{
	#ifdef MATERIAL_DIFFUSE
		diffuse();
	#endif
	
	#ifdef MATERIAL_SPECULAR
		specular(rl_InRay.color * ks.xyz, normalize(normal));
	#endif
	
	#ifdef MATERIAL_TRANSMISSIVE
		transmissive();
	#endif
}




