//
//  simpleLight.rsh
//  Heatray
//
//  Created by Cody White on 1/11/16.
//
//
// Implement a simple light accumulation shader to be attached to any light primitives.

rayattribute vec3 color;
rayattribute bool is_diffuse_bounce;
uniform vec3 kd; // Color of this light.

#ifdef MATERIAL_DIFFUSE_TEXTURE
	uniform sampler2D diffuse_texture;
	varying vec2 tex_coords;
#endif

void main()
{
	vec3 color = rl_InRay.color * kd;
	
	#ifdef MATERIAL_DIFFUSE_TEXTURE
		color *= texture2D(diffuse_texture, tex_coords).zyx;
	#endif
	
	accumulate(color);
}
