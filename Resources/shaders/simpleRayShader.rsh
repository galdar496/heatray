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

#ifdef MATERIAL_SUBSURFACE
	uniform int subsurface;
	uniform primitive subsurfacePrimitive;
#endif

// OpenRL setup shader to specify how many rays at most this shader will emit.
void setup()
{
	// Determine the number of rays that will be emitted by this material.
	int numRaysToEmit = 0;
	
	#ifdef MATERIAL_DIFFUSE
		// Diffuse bounce + shadow rays.
		numRaysToEmit += 1 + Light.count;
	#endif

	#ifdef MATERIAL_SPECULAR
		// Reflection ray.
		numRaysToEmit += 1;
	#endif

	#ifdef MATERIAL_TRANSMISSIVE
		// Reflection and refraction ray.
		numRaysToEmit += 2;
	#endif

	#ifdef MATERIAL_SUBSURFACE
		// Subsurface ray.
		numRaysToEmit += 1;
	#endif

	rl_OutputRayCount[0] = numRaysToEmit;
}

#ifdef MATERIAL_DIFFUSE

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

#ifdef MATERIAL_NORMALMAP
    uniform sampler2D normalmap;
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
        
        // Note that the PDF for cosine-weighted sampling is cos(theta) / pi. This means that
        // the monte-carlo estimator for diffuse bouncing of one ray is:
        //          c * Li
        // where:
        //  c : the diffuse color of the surface material at this point (current ray energy).
        //  Li: the incoming radiance at this point (result of the diffuse bounce).
        //
        // To accurately sample the entire hemisphere, multiple rays must be summed and averaged
        // over the entire surface. Heatray accomplishes this iteratively, meaning that each pass
        // represents a single ray in the hemisphere. All passes are averaged which results in the
        // actual monte-carlo estimator being:
        //          (c / N) * SUM(Li | from 1 to N)
        // where:
        //  N: the number of samples in the hemisphere (render passes).
        //  c and Li are the same as above.
        
        createRay();
        rl_OutRay.direction         = basis * samplePoint;
        rl_OutRay.color             = baseColor;
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

    for (int ii = 0; ii < Light.count; ++ii)
    {
        vec3 lightDir = Light.position[ii] - rl_IntersectionPoint;
        float vecLength = length(lightDir);
        lightDir /= vecLength; // normalize.
        
        float ndotl = dot(lightDir, surfaceNormal);
        
        if (ndotl > 0.0)
        {
            // Determine the angle between the direction of the light and the light's direction.
            float angle = dot(Light.normal[ii], -lightDir);
            
            if (angle > 0.0)
            {
				// Fire a shadow ray at the light source.
                createRay();
				rl_OutRay.direction        = lightDir;
				rl_OutRay.color            = baseColor * angle * ndotl;
                rl_OutRay.maxT             = vecLength;
                rl_OutRay.defaultPrimitive = Light.lightPrimitive[ii];
                rl_OutRay.occlusionTest    = true;
                emitRay();
            }
        }
//        else if (subsurface != 0)
//        {   
//            createRay();
//            rl_OutRay.direction = lightDir;
//            rl_OutRay.color = baseColor;
//            rl_OutRay.prefixPrimitive = subsurfacePrimitive;
//            emitRay();
//        }
    }
    
    if (GI.enabled != 0)
    {
        DiffuseBounce(baseColor, surfaceNormal);
    }
}
#endif

#if defined(MATERIAL_SPECULAR) || defined(MATERIAL_TRANSMISSIVE)

// First 3 components are the specular reflection powers per color channel. The last is the roughness.
uniform vec4 ks;

void Specular(vec3 rayPower, vec3 surfaceNormal)
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
    rl_OutRay.direction = reflect(rl_InRay.direction, surfaceNormal) + offset;
    rl_OutRay.color = rayPower;
    emitRay();
}
#endif

#ifdef MATERIAL_TRANSMISSIVE

// First 3 components are the refractive color per channel, the last is the IOR for this material.
uniform vec4 kt;

void Transmissive()
{
    // Calculate the fresnel coefficients.
    float ref;
    vec3 actualNormal;
    if (rl_FrontFacing)
    {
        ref = 1.0 / kt.w;
        actualNormal = normalize(normal);
    }
    else
    {
        ref = kt.w;
        actualNormal = normalize(-normal);
    }
    
    float cosThetaI = abs(dot(actualNormal, -1.0 * rl_InRay.direction));
    float sinThetaT2 = (ref * ref) * (1.0 - (cosThetaI * cosThetaI));
    float reflectance = 1.0;
    float refraction = 0.0;
    float cosThetaT;
    if (sinThetaT2 < 1.0)
    {
        cosThetaT = sqrt(1.0 - sinThetaT2);
        float reflectanceParal = (ref * cosThetaI - cosThetaT) / (ref * cosThetaI + cosThetaT);
        float reflectanceOrtho = (cosThetaI - ref * cosThetaT) / (cosThetaI + ref * cosThetaT);
        reflectanceOrtho *= reflectanceOrtho;
        reflectanceParal *= reflectanceParal;
        reflectance = 0.5 * (reflectanceOrtho + reflectanceParal);
        refraction = 1.0 - reflectance;
    }
    
    if (reflectance > 0.0)
    {
        Specular(rl_InRay.color * kt.xyz * reflectance, actualNormal);
    }
    
    if (refraction > 0.0)
    {
        vec3 refractedDir = (ref * rl_InRay.direction) + (actualNormal * ((ref * cosThetaI) - cosThetaT));

        createRay();
        rl_OutRay.direction = refractedDir;
        rl_OutRay.color = rl_InRay.color * kt.xyz * refraction;
        if (rl_InRay.isDiffuseBounce)
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
		Diffuse();
	#endif
	
	#ifdef MATERIAL_SPECULAR
		Specular(rl_InRay.color * ks.xyz, normalize(normal));
	#endif
	
	#ifdef MATERIAL_TRANSMISSIVE
		Transmissive();
	#endif
}




