// Display a raytraced image to an OpenGL device. The raytraced image needs to be
// divided by the number of passes that have been done which is stored in the alpha
// channel of the texture.

#extension GL_EXT_gpu_shader4 : require

uniform sampler2D raytracedTexture;
uniform int tonemappingEnabled;
uniform float cameraExposure;
uniform float brightness;
uniform float contrast;
uniform float hue;
uniform float saturation;
uniform float vibrance;
uniform float red;
uniform float green;
uniform float blue;

in vec2 textureCoords;

const float SRGB_ALPHA = 0.055;

// ACES tonemapping adapted from: https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
const mat3 ACESInputMat = mat3(
	vec3(0.59719, 0.07600, 0.02840),
	vec3(0.35458, 0.90834, 0.13383),
	vec3(0.04823, 0.01566, 0.83777)
);

const mat3 ACESOutputMat = mat3(
	vec3(1.60475, -0.10208, -0.00327),
	vec3(-0.53108, 1.10813, -0.07276),
	vec3(-0.07367, -0.00605, 1.07602)
);

vec3 RRTAndODTFit(vec3 v)
{
    vec3 a = v * (v + 0.0245786) - 0.000090537;
    vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return a / b;
}

vec3 LinearToSRGB(vec3 linear) {
    vec3 result;
	for (int channel = 0; channel < 3; ++channel) {
		if (linear[channel] <= 0.0031308) {
			result[channel] = 12.92 * linear[channel];
		} else {
			result[channel] = 1.055 * pow(linear[channel], 1.0 / 2.4) - SRGB_ALPHA;
		}
	}

	return result;
}

vec3 SRGBToLinear(vec3 srgb) {
    vec3 result;
	for (int channel = 0; channel < 3; ++channel) {
		if (srgb[channel] <= 0.04045) {
			result[channel] = srgb[channel] / 12.92;
		} else {
			result[channel] = pow((srgb[channel] + SRGB_ALPHA) / (1.0 + SRGB_ALPHA), 2.4);
		}
	}

	return result;
}

void main()
{
	// raytraced texture is rgb of all accumulated passes and w = # of passes performed.
	vec4 result = texture2D(raytracedTexture, textureCoords.xy);
	vec3 finalColor = result.xyz / result.w;

	// Apply the color processing pipeline.
	{
		if (tonemappingEnabled == 1) {
			// Perform ACES tonemapping. Incoming color is in linear space.
			finalColor = LinearToSRGB(finalColor);
			finalColor = ACESInputMat * finalColor;
			finalColor = RRTAndODTFit(finalColor);
			finalColor = ACESOutputMat * finalColor;
			finalColor = SRGBToLinear(finalColor);
		}

		// Colors are expected to be linear.

		// Brightness/contrast.
		{
			finalColor = (finalColor.rgb - 0.5) * contrast + 0.5 + brightness;
		}

		// Hue/Saturation/Vibrance.
		{
			// First, convert from RGB->HSV (hue/saturation/value).
			vec3 hsv;
			{
				const vec4 k = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
				vec4 p = mix(vec4(finalColor.bg, k.wz), vec4(finalColor.gb, k.xy), step(finalColor.b, finalColor.g));
				vec4 q = mix(vec4(p.xyw, finalColor.r), vec4(finalColor.r, p.yzx), step(p.x, finalColor.r));

				float d = q.x - min(q.w, q.y);
				const float e = 1.0e-10;
				hsv = vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
			}

			hsv.x *= hue;
			hsv.y *= saturation;

			{
				// For vibrance, we use a sqrt curve so that we have the largest impact on less-saturated pixels.
				float mapped_saturation = sqrt(hsv.y) * vibrance;
				hsv.y *= 1.0 + mapped_saturation;
			}

			// HSV->RGB conversion.
			{
				const vec4 k = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
				vec3 p = abs(fract(hsv.xxx + k.xyz) * 6.0 - k.www);
				finalColor.rgb = hsv.z * mix(k.xxx, clamp(p - k.xxx, 0.0, 1.0), hsv.y);
			}
		}

		// Alter the RGB levels.
		{
			finalColor.r *= red;
			finalColor.g *= green;
			finalColor.b *= blue;
		}

		// Finally, apply exposure compensation.
		finalColor *= cameraExposure;
	}

	finalColor = LinearToSRGB(finalColor);

	gl_FragColor = vec4(finalColor, 1.0);
}