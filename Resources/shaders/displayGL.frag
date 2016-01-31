//
//  Display.frag
//  Heatray
//
//  Created by Cody White on 1/31/16.
//
//

// Display a raytraced image to an OpenGL device. The raytraced image needs to be
// divided by the number of passes that have been done, have exposure compensation applied
// and perform proper gamma correction before final display.

uniform sampler2D raytracedTexture; // Raytraced output texture. Needs to be divided in order to display.

// Includes both the number of passes performed (over 1 for multiplication).
uniform float passDivisor;

const float gammaCorrection = 2.2;

void main()
{
    vec3 finalColor = texture2D(raytracedTexture, gl_TexCoord[0].st).xyz;

    // Get the actual color to display for the current pass.
    finalColor *= passDivisor;
    
    // Apply gamma correction.
    finalColor = pow(finalColor, vec3(1.0 / gammaCorrection));
    
    gl_FragColor = vec4(finalColor, 1.0);
}
