//
//  diffuse.rsh
//  Heatray
//
//  Created by Cody White on 5/10/14.
//
//
// Implement GI via path tracing.

// Uniform block which contains a texture full of random values between 0 and 1.
// These are used to determine a new sample in the hemisphere for GI rays.
uniformblock GI
{
    sampler2D random_values; // Texture full of random values.
    int enabled;             // Flag to turn on/off GI (path tracing).
};

uniform float bounce_probablility; // This is the probability that this ray will make a diffuse bounce (kd.x + kd.y + kd.z) / 3.

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
    vec4 rand = texture2D(GI.random_values, noise2(Light.positions[0].xy * rl_FrameCoord.xy));
    
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





