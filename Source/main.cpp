//
//  main.cpp
//  Heatray
//
//  Created by Cody White on 5/9/14.
//
//

#include <GL/glew.h>

#if defined(_WIN32) || defined(_WIN64)
    #include <GLUT/glut.h>
    #include <gl/GL.h>
#else
    #include <GLUT/GLUT.h>
    #include <OpenGL/GL.h>
#endif
#include "raytracer/Raytracer.h"
#include "raytracer/Pixels.h"
#include "utility/Timer.h"

#include <iostream>
#include <sstream>

// Raytracer exists as a global variable to be accessed by all
// functions in the main file.
Raytracer raytracer;
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F_ARB, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    glViewport(0, 0, width, height);
}

void resizeWindow(int width, int height)
{
    raytracer.resize(width, height);
    pixels.resize(width, height);

    resizeGLData(width, height);
}

void render()
{
    std::stringstream windowTitle;
    int render_passed_performed = raytracer.getNumPassesPerformed();
    windowTitle << "Heatray - Pass " << render_passed_performed;
    glutSetWindowTitle(windowTitle.str().c_str());

    // Perform the actual rendering of the raytracer into a pixel buffer.
    raytracer.render(pixels);

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
    raytracer.update(timer.dt());
    glutPostRedisplay();
}

void shutdown()
{
    pixels.destroy();
    raytracer.destroy();

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
    
    raytracer.getKeys().set(key);
}

void keyReleased(unsigned char key, int mouseX, int mouseY)
{
    // Certain keys (keys that aren't held down) may not be properly processed if they're reset too soon (such as enabling GI).
    // Ask the raytracer which keys those might be to make sure we don't prematurely reset it. In those cases, the raytracer will handle
    // resetting any special keys.
    if (!raytracer.isSpecialKey(key))
    {
        raytracer.getKeys().reset(key);
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
    if (!raytracer.initialize(config_file, screen_width, screen_height))
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
    
    // Make sure that OpenGL doesn't automatically clamp our display texture. This is because the raytraced image
    // that will be stored in it is actually an accumulation of every ray that has gone through a given pixel.
    // Therefore each pixel value will quickly move beyond 1.0.
    glClampColorARB(GL_CLAMP_FRAGMENT_COLOR, GL_FALSE);

    timer.start();
    glutMainLoop();
    
    return 0;
}
