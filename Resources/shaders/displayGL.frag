// Display a raytraced image to an OpenGL device. The raytraced image needs to be
// divided by the number of passes that have been done which is stored in the alpha
// channel of the texture.

#extension GL_EXT_gpu_shader4 : require

uniform sampler2D raytracedTexture;

void main()
{
	vec4 result = texture2D(raytracedTexture, gl_TexCoord[0].st);
	vec3 finalColor = result.xyz / result.w;
	gl_FragColor = vec4(finalColor, 1.0);
}