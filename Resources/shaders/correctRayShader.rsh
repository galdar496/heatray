//
//  correctRayShader.rsh
//  Heatray
//
//  Created by Cody White on 8/04/17.
//
//

rayattribute vec3 color;           // Color value to use for the final ray accumulation.
rayattribute bool isDiffuseBounce; // If true, this ray is a diffuse bounce used for path tracing.

varying vec3 normal;
varying vec2 texCoords;

// Uniform values.
uniformblock Light
{
	// MAX_LIGHTS is defined by the Heatray source code.
	
	vec3  position[MAX_LIGHTS]; // Position of this light in world-space.
	vec3  normal[MAX_LIGHTS];    // Direction this light is shining.
	
	int count; // How many lights are currently active.
	primitive lightPrimitive[MAX_LIGHTS]; // The RL primitive for the lights.
};

uniform primitive defaultPrim; // Default primitive to use if the ray misses any geometric primitives.

// OpenRL setup shader to specify how many rays at most this shader will emit.
void setup()
{
	// Determine the number of rays that will be emitted by this material.
	int numRaysToEmit = 1 + Light.count;
	rl_OutputRayCount[0] = numRaysToEmit;
}

uniform vec3 kd;                  // Diffuse color.
uniform float bounceProbablility; // This is the probability that this ray will make a diffuse bounce (kd.x + kd.y + kd.z) / 3.

// Uniform block which contains a texture full of random values between 0 and 1.
// These are used to determine a new sample in the hemisphere for GI rays.
uniformblock GI
{
    sampler2D randomValues;  // Texture full of random values between 0 and 1.
    int enabled;             // Flag to turn on/off GI (path tracing).
};

varying vec3 tangent;
varying vec3 bitangent;

#ifdef MATERIAL_DIFFUSE_TEXTURE
    uniform sampler2D diffuseTexture; // Diffuse texture to use for shading.
#endif

vec3 GetNormal()
{
	#ifdef MATERIAL_NORMALMAP
		// Convert the tangent-space normal into world-space.
		vec3 norm = texture2D(normalmap, texCoords).zyx * 2.0 - 1.0;
		return mat3(normalize(tangent), normalize(bitangent), normalize(normal)) * norm;
	#else
		return normalize(normal);
	#endif
}

float twoPI = 2.0 * 3.14159;

// Generate a random sample in the hemisphere using cosine-weighted sampling. This random
// sample is represented in tangent space.
vec3 CosineSampleHemisphere(vec3 rand)
{
    // Generate a random point on a disk and and project them up into the hemisphere.
    float r = sqrt(rand.x);
    float theta = twoPI * rand.y;
    float x = r * cos(theta);
    float y = r * sin(theta);
    return vec3(x, y, sqrt(max(0.0, 1.0 - rand.x)));
}

// Generate a diffuse bounce ray and emit it.
void DiffuseBounce(vec3 baseColor, vec3 surfaceNormal)
{
    vec3 rand = texture2D(GI.randomValues, noise2(Light.position[0].xy * rl_FrameCoord.xy)).xyz;
    
    // Russian roulette.
    if (rand.z <= bounceProbablility)
    {
        // Construct a basis matrix out of the tangent, bitangent, and normal. This matrix is
        // used to transform the random hemisphere sample into world space.
        mat3 basis = mat3(normalize(tangent), normalize(bitangent), surfaceNormal);
        
        vec3 samplePoint = CosineSampleHemisphere(rand);
        
        createRay();
        rl_OutRay.direction         = basis * samplePoint;
        rl_OutRay.color             = baseColor; // PDF of cosine-weighted sampling is cos(theta) / pi which cancels out to 1
        rl_OutRay.isDiffuseBounce   = true;
        emitRay();
    }
}

void Diffuse()
{
    vec3 baseColor     = rl_InRay.color * kd;
    vec3 surfaceNormal = GetNormal();
	
	#ifdef MATERIAL_DIFFUSE_TEXTURE
        // Texture coordinates are revered because the texture is read backwards from FreeImage.
		vec3 texColor = texture2D(diffuseTexture, texCoords).zyx;
        baseColor *= texColor;
	#endif
    
	// Perform next-event estimation.
    for (int ii = 0; ii < Light.count; ++ii)
    {
        vec3 lightDir = Light.position[ii] - rl_IntersectionPoint;
        float vecLength = length(lightDir);
        lightDir /= vecLength; // normalize.
        
        float ndotl = dot(lightDir, surfaceNormal);
        
        if (ndotl > 0.0)
        {
            // Determine the angle between the direction to the light and the light's direction.
            float angle = dot(Light.normal[ii], -lightDir);
            
            if (angle > 0.0)
            {
				// Fire a shadow ray at the light source.
                createRay();
				rl_OutRay.direction        = lightDir;
				rl_OutRay.color            = ((baseColor * angle * ndotl) / 3.14159) / float(Light.count); // PDF is 1
                rl_OutRay.maxT             = vecLength;
                rl_OutRay.defaultPrimitive = Light.lightPrimitive[ii];
                rl_OutRay.occlusionTest    = true;
				rl_OutRay.isDiffuseBounce  = false;
                emitRay();
            }
        }
    }

	if (GI.enabled == 1)
	{
		DiffuseBounce(baseColor, surfaceNormal);
	}
}

void main()
{
	Diffuse();
}




