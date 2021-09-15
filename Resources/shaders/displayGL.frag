// Display a raytraced image to an OpenGL device. The raytraced image needs to be
// divided by the number of passes that have been done which is stored in the alpha
// channel of the texture.

#extension GL_EXT_gpu_shader4 : require

uniform sampler2D raytracedTexture;
uniform int tonemappingEnabled;
uniform float cameraExposure;

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

void main()
{
	// raytraced texture is rgb of all accumulated passes and w = # of passes performed.
	vec4 result = texture2D(raytracedTexture, gl_TexCoord[0].st);
	vec3 finalColor = result.xyz / result.w;

	if (tonemappingEnabled == 1) {
		// Perform ACES tonemapping.
		finalColor = ACESInputMat * finalColor;
		finalColor = RRTAndODTFit(finalColor);
		finalColor = ACESOutputMat * finalColor;
	}

	finalColor *= cameraExposure;
	gl_FragColor = vec4(finalColor, 1.0);
}