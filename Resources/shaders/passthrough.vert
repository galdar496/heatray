//
//  passthrough.vert
//  Heatray
//
//  Created by Cody White on 5/10/14.
//
//

attribute vec3 positionAttribute;
attribute vec3 normalAttribute;
attribute vec2 texCoordAttribute;
attribute vec3 tangentAttribute;
attribute vec3 bitangentAttribute;

varying vec3 normal;
varying vec2 texCoords;
varying vec3 tangent;
varying vec3 bitangent;

void main()
{
    rl_Position = vec4(positionAttribute, 1.0);
    
    // Varyings.
    normal      = normalAttribute;
    texCoords   = texCoordAttribute;
    tangent     = tangentAttribute;
    bitangent   = bitangentAttribute;
}