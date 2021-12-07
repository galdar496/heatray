//
//  main.cpp
//  Heatray
//
// 
//
//

#include "HeatrayRenderer/HeatrayRenderer.h" // Must be included first for GL/glew reasons.
#include "Utility/ConsoleLog.h"
#include "Utility/ImGuiLog.h"
#include "Utility/TextureLoader.h"

#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include "../3rdParty/glm/glm/glm.hpp"

#include <assert.h>

#if defined(_DEBUG)
    #define HEATRAY_DEBUG 1
#else
    #define HEATRAY_DEBUG 0
#endif // defined(_DEBUG)

const std::string kVersion      = "4.0 - Build 0x0003";
const std::string kWindowTitle  = "Heatray " + kVersion;

bool mousePositionValid = false;
bool isMovingCamera = false;
glm::vec2 previousMousePosition;
glm::ivec2 previousWindowSize;

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

void glfwErrorCallback(int error, const char* description)
{
    LOG_ERROR("GLFW error %d: %s\n", error, description);
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (previousMousePosition.x >= HeatrayRenderer::UI_WINDOW_WIDTH) {
        heatray.adjustCamera(0.0f, 0.0f, static_cast<float>(-yoffset));
    }
}

void glfwPathDropCallback(GLFWwindow* window, int pathCount, const char* paths[])
{
    if (pathCount) {
        // Is this an env map or a scene file?
        static constexpr char * ENV_FORMATS[] = { ".exr", ".hdr" };
        static constexpr size_t NUM_ENV_FORMATS = sizeof(ENV_FORMATS) / sizeof(ENV_FORMATS[0]);
    
        std::string path(paths[0]);
        for (size_t iFormat = 0; iFormat < NUM_ENV_FORMATS; ++iFormat) {
            if (path.find(ENV_FORMATS[iFormat]) != std::string::npos) {
                heatray.changeEnvironment(path);
                return;
            }
        }

        // If we've gotten this far then we assume the path is a scene file.
        heatray.changeScene(path, true);
        heatray.resetRenderer();
    }
}

int main(int argc, char **argv)
{
    util::ConsoleLog::install();

    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        return 1;
    }

    // GL 4.6 + GLSL 460
    const char* glsl_version = "#version 460";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, (HEATRAY_DEBUG ? GLFW_TRUE : GLFW_FALSE));
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(kDefaultWindowWidth, kDefaultWindowHeight, kWindowTitle.c_str(), nullptr, nullptr);
    if (window == nullptr) {
        return 1;
    }

    // Setup the window icon.
    {
        int width, height, channelCount = 0;
        uint8_t* pixels = util::loadLDRTexturePixels("Resources/logo.png", width, height, channelCount);

        GLFWimage icon;
        icon.pixels = pixels;
        icon.width = width;
        icon.height = height;
        glfwSetWindowIcon(window, 1, &icon);
        free(pixels);
    }

    glfwMakeContextCurrent(window);

    // Enable vsync
    glfwSwapInterval(1);

    // Callbacks
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetDropCallback(window, glfwPathDropCallback);

    // Handles GLEW init as well.
    heatray.init(kDefaultWindowWidth, kDefaultWindowHeight);
    heatray.resize(kDefaultWindowWidth * kCheesyMultiplier, kDefaultWindowHeight * kCheesyMultiplier);

    // ImGui setup code.
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);

        util::ImGuiLog::install();
    }

    previousWindowSize = glm::ivec2(kDefaultWindowWidth, kDefaultWindowHeight);

    // Main loop.
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Resize.
        {
            glm::ivec2 newWindowSize;
            glfwGetWindowSize(window, &newWindowSize.x, &newWindowSize.y);
            if (newWindowSize != previousWindowSize) {
                if ((newWindowSize.x > 0) &&
                    (newWindowSize.y > 0)) {
                    heatray.resize(newWindowSize.x * kCheesyMultiplier, newWindowSize.y * kCheesyMultiplier);
                }
                previousWindowSize = newWindowSize;
            }
        }

        // Handle mouse.
        {
            // Buttons.
            {
                if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                    if (!isMovingCamera) {
                        isMovingCamera = true;
                        mousePositionValid = false;
                    }
                } else {
                    isMovingCamera = false;
                }
            }

            // Position.
            {
                double x, y;
                glfwGetCursorPos(window, &x, &y);

                // If we're inside of the rendering window then let the renderer know that the user wants to move the camera.
                if (x >= HeatrayRenderer::UI_WINDOW_WIDTH) {
                    if (isMovingCamera) {
                        if (!mousePositionValid) {
                            previousMousePosition = glm::vec2(x, y);
                            mousePositionValid = true;
                        } else {
                            glm::vec2 mouseDelta = glm::vec2(x, y) - previousMousePosition;
                            heatray.adjustCamera(mouseDelta.x, mouseDelta.y, 0.0f);							
                        }
                    }
                }

                previousMousePosition = glm::vec2(x, y);
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        heatray.render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(window);
    }

    heatray.destroy();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}
