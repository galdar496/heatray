//
//  main.cpp
//  Heatray
//
// 
//
//

#include "HeatrayRenderer/HeatrayRenderer.h" // Must be included first for GL/glew reasons.

#include <GLUT/freeglut.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glut.h"
#include "imgui/imgui_impl_opengl2.h"

#include <assert.h>
#include <iostream>

const std::string kVersion      = "4.0";
const std::string kWindowTitle  = "Heatray " + kVersion;

namespace {
    // The renderer exists as a global variable to be accessed by all
    // functions in the main file.
    HeatrayRenderer heatray;

	constexpr GLint kDefaultWindowWidth = 800 + HeatrayRenderer::UI_WINDOW_WIDTH;
    constexpr GLint kDefaultWindowHeight = 800;
} // namespace.

#if defined(_WIN32) || defined(_WIN64)
const int kCheesyMultiplier = 1;
#else
const int kCheesyMultiplier = 2;
#endif

void resizeWindow(int width, int height)
{
    heatray.resize(width * kCheesyMultiplier, height * kCheesyMultiplier);
    ImGui_ImplGLUT_ReshapeFunc(width * kCheesyMultiplier, height * kCheesyMultiplier);
}

void render()
{  
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGLUT_NewFrame();

    heatray.render();
    
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    glutSwapBuffers();
}

void update()
{
    glutPostRedisplay();

    heatray.handlePendingFileLoads();
}

void shutdown()
{
    heatray.destroy();

    // ImGui deinit.
    {
        ImGui_ImplOpenGL2_Shutdown();
        ImGui_ImplGLUT_Shutdown();
        ImGui::DestroyContext();
    }
}

void motion(int x, int y)
{
    ImGui_ImplGLUT_MotionFunc(x * kCheesyMultiplier, y * kCheesyMultiplier);
}

void mouseMove(int button, int state, int x, int y)
{
    ImGui_ImplGLUT_MouseFunc(button, state, x * kCheesyMultiplier, y * kCheesyMultiplier);
}

#ifdef __FREEGLUT_EXT_H__
void mouseWheel(int button, int dir, int x, int y)
{
	ImGui_ImplGLUT_MouseWheelFunc(button, dir, x, y);
}
#endif // __FREEGLUT_EXT_H__

void keyboardPressed(unsigned char c, int x, int y)
{
    ImGui_ImplGLUT_KeyboardFunc(c, x * kCheesyMultiplier, y * kCheesyMultiplier);
}

void keyboardUp(unsigned char c, int x, int y)
{
    ImGui_ImplGLUT_KeyboardUpFunc(c, x * kCheesyMultiplier, y * kCheesyMultiplier);
}

void specialPressed(int key, int x, int y)
{
    ImGui_ImplGLUT_SpecialFunc(key, x * kCheesyMultiplier, y * kCheesyMultiplier);
}

void specialUp(int key, int x, int y)
{
    ImGui_ImplGLUT_SpecialUpFunc(key, x * kCheesyMultiplier, y * kCheesyMultiplier);
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitWindowPosition(100,100);
    glutInitWindowSize(kDefaultWindowWidth, kDefaultWindowHeight);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
    glutCreateWindow(kWindowTitle.c_str());

    glutReshapeFunc(resizeWindow);
    glutDisplayFunc(render);
    glutIdleFunc(update);
    glutMotionFunc(motion);
    glutMouseFunc(mouseMove);
#ifdef __FREEGLUT_EXT_H__
	glutMouseWheelFunc(mouseWheel);
#endif // __FREEGLUT_EXT_H__
    glutKeyboardFunc(keyboardPressed);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialPressed);
    glutSpecialUpFunc(specialUp);

    heatray.init(kDefaultWindowWidth, kDefaultWindowHeight);

    // ImGui setup code.
    if (true) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        ImGui_ImplGLUT_Init();
        ImGui_ImplOpenGL2_Init();
    }

    glutMainLoop();
    
    return 0;
}
