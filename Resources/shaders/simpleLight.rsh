//
//  simpleLight.rsh
//  Heatray
//
//  Created by Cody White on 1/11/16.
//
//
// Implement a simple light accumulation shader to be attached to any light primitives.

rayattribute vec3 color;
rayattribute bool isDiffuseBounce;
uniform vec3 kd; // Color of this light.

#ifdef MATERIAL_DIFFUSE_TEXTURE
	uniform sampler2D diffuseTexture;
	varying vec2 texCoords;
#endif

void main()
{
	vec3 color = rl_InRay.color * kd;
	
	#ifdef MATERIAL_DIFFUSE_TEXTURE
        // Texture coordinates are revered because the texture is read backwards from FreeImage.
		color *= texture2D(diffuseTexture, texCoords).zyx;
	#endif
	
	accumulate(color);
}
