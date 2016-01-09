//
//  main.cpp
//  Heatray
//
//  Created by Cody White on 5/9/14.
//
//

#if defined(_WIN32) || defined(_WIN64)
    #include <GL/glew.h>
    #include <GLUT/glut.h>
    #include <gl/GL.h>
#else
    #include <GLUT/GLUT.h>
#endif
#include "raytracer/World.h"
#include "raytracer/Pixels.h"
#include "utility/Timer.h"

#include <iostream>
#include <sstream>

// World exists as a global variable to be accessed by all
// functions in the main file.
World world; 
Pixels pixels;          // Pixel object which contains the rendered pixels from the raytracer.
GLuint pixelBuffer;     // PBO to use to put the pixels on the GPU.
GLuint displayTexture;  // Texture which contains the final result for display.
util::Timer timer;

#define GLUT_KEY_ESCAPE 27

void resizeGLData(int width, int height)
{
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pixelBuffer);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height * sizeof(float) * Pixels::NUM_PIXEL_CHANNELS, nullptr, GL_STREAM_DRAW);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    glBindTexture(GL_TEXTURE_2D, displayTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    glViewport(0, 0, width, height);
}

void resizeWindow(int width, int height)
{
    world.resize(width, height);
    pixels.resize(width, height);

    resizeGLData(width, height);
}

void render()
{
    std::stringstream windowTitle;
    int render_passed_performed = world.getNumPassesPerformed();
    windowTitle << "Heatray - Pass " << render_passed_performed;
    glutSetWindowTitle(windowTitle.str().c_str());

    // Perform the actual rendering of the world into a pixel buffer.
    world.render(pixels);

    size_t width, height;
    pixels.getDimensions(width, height);
    
    const float *image_pixels = pixels.mapPixelData();
    
    // Copy the data into a PBO and upload it to a texture for rendering.
    glBindTexture(GL_TEXTURE_2D, displayTexture);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pixelBuffer);
    glBufferSubData(GL_PIXEL_UNPACK_BUFFER, 0, width * height * sizeof(float) * Pixels::NUM_PIXEL_CHANNELS, image_pixels); 

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), GL_RGB, GL_FLOAT, nullptr);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    // The raytraced result is actually an accumulation of every pass so far. In order to properly display it,
    // it must be averaged by the number of passes that have been ran to far.
    // Utilize the rasterization hardware to perform the averaging.
    float inv_num_passes = 1.0f / static_cast<float>(render_passed_performed);
    glColor3f(inv_num_passes, inv_num_passes, inv_num_passes);
    glBegin(GL_QUADS);
        glTexCoord2d(0.0, 0.0); glVertex2f(-1.0f, -1.0f);
        glTexCoord2d(1.0, 0.0); glVertex2f(1.0f, -1.0f);
        glTexCoord2d(1.0, 1.0); glVertex2f(1.0f, 1.0f);
        glTexCoord2d(0.0, 1.0); glVertex2f(-1.0f, 1.0f);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    pixels.unmapPixelData();

    glutSwapBuffers();
}

void update()
{
    world.update(timer.dt());
    glutPostRedisplay();
}

void shutdown()
{
    pixels.destroy();
    world.destroy();

    glDeleteBuffers(1, &pixelBuffer);
    glDeleteTextures(1, &displayTexture);
}

void keyPressed(unsigned char key, int mouseX, int mouseY)
{
    if (key == GLUT_KEY_ESCAPE)
    {
        shutdown();
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

    GLenum result = glewInit();

    pixels.resize(screen_width, screen_height);

    // Create the pixel-pack buffer and gl texture to use for displaying the raytraced result.
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_BLEND);

    glGenBuffers(1, &pixelBuffer);
    glGenTextures(1, &displayTexture);

    // Setup the texture state.
    glBindTexture(GL_TEXTURE_2D, displayTexture);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    resizeGLData(screen_width, screen_height);

    timer.start();
    glutMainLoop();
    
    return 0;
}
