//
//  main.cpp
//  Heatray
//
//  Created by Cody White on 5/9/14.
//
//

#if defined(_WIN32) || defined(_WIN64)
    #include <GLUT/glut.h>
#else
    #include <GLUT/GLUT.h>
#endif
#include "raytracer/World.h"
#include "utility/Timer.h"
#include "gfx/Texture.h"

#include <iostream>
#include <sstream>

// World exists as a global variable to be accessed by all
// functions in the main file.
World world;
util::Timer timer;
RLbuffer pixels;

#define GLUT_KEY_ESCAPE 27

void resizeWindow(int width, int height)
{
    world.resize(width, height);
    
    if (pixels != RL_NULL_BUFFER)
    {
        rlDeleteBuffers(1, &pixels);
    }
    
    // Allocate a pixel-pack buffer with enough memory to store a 4-channel
    // floating-point image.
    rlGenBuffers(1, &pixels);
    rlBindBuffer(RL_PIXEL_PACK_BUFFER, pixels);
    rlBufferData(RL_PIXEL_PACK_BUFFER, width * height * sizeof(float) * 4, NULL, RL_STATIC_DRAW);
}

void render()
{
    std::stringstream windowTitle;
    windowTitle << "Heatray - Pass " << world.getNumPassesPerformed();
    glutSetWindowTitle(windowTitle.str().c_str());

    // Perform the actual rendering of the world into a texture.
    const gfx::Texture *image = world.render();
    
    // Display the generated frame to the screen.
    rlBindBuffer(RL_PIXEL_PACK_BUFFER, pixels);
    rlBindTexture(RL_TEXTURE_2D, image->getTexture());
    rlGetTexImage(RL_TEXTURE_2D, 0, RL_RGBA, RL_FLOAT, NULL);
    float *image_pixels = static_cast<float *>(rlMapBuffer(RL_PIXEL_PACK_BUFFER, RL_READ_ONLY));

    glDrawPixels(image->width(), image->height(), GL_RGBA, GL_FLOAT, image_pixels);
    rlUnmapBuffer(RL_PIXEL_PACK_BUFFER);

    glutSwapBuffers();
}

void update()
{
    world.update(timer.dt());
    glutPostRedisplay();
}

void keyPressed(unsigned char key, int mouseX, int mouseY)
{
    if (key == GLUT_KEY_ESCAPE)
    {
        rlDeleteBuffers(1, &pixels);
        world.destroy();
        exit(0);
    }
    
    world.getKeys().set(key);
}

void keyReleased(unsigned char key, int mouseX, int mouseY)
{
    // Certain keys (keys that aren't held down) may not be properly processed if they're reset too soon (such as enabling GI).
    // Ask the world which keys those might be to make sure we don't prematurely reset it. In those cases, the world will handle
    // resetting any special keys.
    if (!world.isSpecialKey(key))
    {
        world.getKeys().reset(key);
    }
}

void shutdown()
{
    world.destroy();
}

int main(int argc, char **argv)
{
    std::string config_file = "defaultconfig.xml";
    if (argc > 1)
    {
        // A config file was specified on the command line, use that instead of the default.
        config_file = argv[1];
    }
    
    RLint screen_width, screen_height;
    if (!world.initialize(config_file, screen_width, screen_height))
    {
        std::cout << "Unable to properly initialize Heatray, exiting..." << std::endl;
        exit(1);
    }
    
    pixels = RL_NULL_BUFFER;
    
    glutInit(&argc, argv);
    glutInitWindowPosition(100,100);
    glutInitWindowSize(screen_width, screen_height);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
    glutCreateWindow("Heatray");

    glutReshapeFunc(resizeWindow);
    glutDisplayFunc(render);
    glutKeyboardFunc(keyPressed);
    glutKeyboardUpFunc(keyReleased);
    glutIdleFunc(update);

    timer.start();
    glutMainLoop();
    
    return 0;
}
