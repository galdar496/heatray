//
//  HeatrayRenderer.h
//  Heatray
//
//  This is the main API to the pathtracer including scene loading, shader creation,
//  and rendering. This connects both the OpenRL-based path tracer with the OpenGL-based
//  frame visualization.
//

#pragma once

#include <glew/GL/glew.h>
#if defined(_WIN32) || defined(_WIN64)
    #include <gl/GL.h>
#else
    #define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED
    #include <OpenGL/gl3.h>
    #include <OpenGL/gl3ext.h>
#endif

#include "FlyCamera.h"
#include "OrbitCamera.h"
#include "PassGenerator.h"
#include "Scene/Scene.h"

#include <Utility/FileIO.h>
#include <Utility/AABB.h>

#include <glm/glm/mat4x4.hpp>
#include <glm/glm/gtx/euler_angles.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

#include <atomic>
#include <memory>

// Forward declarations.
class Material;

class HeatrayRenderer
{
public:
    HeatrayRenderer()
    {
        m_totalPasses = m_renderOptions.maxRenderPasses * (m_renderOptions.enableInteractiveMode ? (m_renderOptions.kInteractiveBlockSize.x * m_renderOptions.kInteractiveBlockSize.y) : 1);
    }
    ~HeatrayRenderer() = default;

    struct WindowParams
    {
        GLint width    = -1; // In pixels.
        GLint height   = -1; // In pixels.
    };

    //-------------------------------------------------------------------------
    // Initialize Heatray for use given a specific window width and height
    // (in pixels).
    bool init(const GLint renderWidth, const GLint renderHeight);

    //-------------------------------------------------------------------------
    // Tear down Heatray and delete all resources.
    void destroy();

    //-------------------------------------------------------------------------
    // Resize the rendering window and any associated window-sized data.
    void resize(const GLint newWidth, const GLint newHeight);

    //-------------------------------------------------------------------------
    // Load a new scene from disk. If 'moveCamera' is true, the camera will be
    // moved to a new location based on the size of the new scene.
    void changeScene(const std::string &sceneName, bool moveCamera);

    //-------------------------------------------------------------------------
    // Load a new environment map to use for image-based lighting.
    void changeEnvironment(const std::string &envMapPath);

    //-------------------------------------------------------------------------
    // Render a frame.
    void render();

    //-------------------------------------------------------------------------
    // Move the orbital camera. Useful for mouse-style controls.
    void adjustCamera(const float phiDelta, const float thetaDelta, const float distanceDelta);

    //-------------------------------------------------------------------------
    // Reset the renderer's internal state.
    void resetRenderer();

    // Fixed pixel width of the ImGui UI.
    static constexpr size_t UI_WINDOW_WIDTH = 500;

private:
    // This class is NOT copyable (move is ok).
    HeatrayRenderer(const HeatrayRenderer& other) = delete;
    HeatrayRenderer& operator=(const HeatrayRenderer& other) = delete;

    void resizeGLData();
    void generateSequenceVisualizationData(int sequenceIndex, int renderPasses, bool aperture);
    bool renderUI();
    void saveScreenshot();

    void writeSessionFile(const std::string_view filename);
    void readSessionFile(const std::string_view filename);

    struct PostProcessingParams {
        bool tonemapping_enabled = false;
        float exposure = 0.0f;
        float brightness = 0.0f;
        float contrast = 1.0f;
        float hue = 1.0f;
        float saturation = 1.0f;
        float vibrance = 0.0f;
        float red = 1.0f;
        float green = 1.0f;
        float blue = 1.0f;
        float vignetteIntensity = 0.0f;
        float vignetteFalloff = 1.0f;
    } m_post_processing_params;

    // Object which handles final display of the raytraced pixels via a custom fragment shader.
    struct DisplayProgram
    {
        GLuint program = 0;
        GLuint fragShader = 0;
        GLuint vertexShader = 0;

        GLuint vbo = 0;
        GLuint vao = 0;

        GLint displayTextureLocation = -1;
        GLint tonemappingEnabledLocation = -1;
        GLint cameraExposureLocation = -1;
        GLint brightnessLocation = -1;
        GLint contrastLocation = -1;
        GLint hueLocation = -1;
        GLint saturationLocation = -1;
        GLint vibranceLocation = -1;
        GLint redLocation = -1;
        GLint greenLocation = -1;
        GLint blueLocation = -1;
        GLint vignetteIntensityLocation = -1;
        GLint vignetteFalloffLocation = -1;
        GLint xStartLocation = -1;

        void init()
        {
            GLint success = 0;
            {
                fragShader = glCreateShader(GL_FRAGMENT_SHADER);
                std::string shaderSource;
                util::readTextFile("Resources/Shaders/displayGL.frag", shaderSource);
                const char* source = shaderSource.c_str();
                glShaderSource(fragShader, 1, &source, nullptr);
                glCompileShader(fragShader);
                glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
                if (success != GL_TRUE) {
                    GLsizei log_length = 0;
                    GLchar  log[1024];
                    glGetShaderInfoLog(fragShader, sizeof(log), &log_length, log);
                    LOG_ERROR("Unable to compile display frag shader: \n\t%s", log);
                }
                assert(success == GL_TRUE);
            }

            {
                vertexShader = glCreateShader(GL_VERTEX_SHADER);
                std::string shaderSource;
                util::readTextFile("Resources/Shaders/displayGL.vert", shaderSource);
                const char* source = shaderSource.c_str();
                glShaderSource(vertexShader, 1, &source, nullptr);
                glCompileShader(vertexShader);
                success = 0;
                glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
                if (success != GL_TRUE) {
                    GLsizei log_length = 0;
                    GLchar  log[1024];
                    glGetShaderInfoLog(vertexShader, sizeof(log), &log_length, log);
                    LOG_ERROR("Unable to compile display vertex shader: \n\t%s", log);
                }
                assert(success == GL_TRUE);
            }

            // Create the display program.
            program = glCreateProgram();
            glAttachShader(program, fragShader);
            glAttachShader(program, vertexShader);
            glLinkProgram(program);
            glGetProgramiv(program, GL_LINK_STATUS, &success);
            assert(success == GL_TRUE);

            // Read the necessary uniform locations.
            displayTextureLocation = glGetUniformLocation(program, "raytracedTexture");
            tonemappingEnabledLocation = glGetUniformLocation(program, "tonemappingEnabled");
            cameraExposureLocation = glGetUniformLocation(program, "cameraExposure");
            brightnessLocation = glGetUniformLocation(program, "brightness");
            contrastLocation = glGetUniformLocation(program, "contrast");
            hueLocation = glGetUniformLocation(program, "hue");
            saturationLocation = glGetUniformLocation(program, "saturation");
            vibranceLocation = glGetUniformLocation(program, "vibrance");
            redLocation = glGetUniformLocation(program, "red");
            greenLocation = glGetUniformLocation(program, "green");
            blueLocation = glGetUniformLocation(program, "blue");
            vignetteIntensityLocation = glGetUniformLocation(program, "vignetteIntensity");
            vignetteFalloffLocation = glGetUniformLocation(program, "vignetteFalloff");
            xStartLocation = glGetUniformLocation(program, "xStart");

            // Create the display VBO.
            {
                // NOTE: we create a empty VBO because the vertices are figured out in-shader.
                glGenVertexArrays(1, &vao);
                glBindVertexArray(vao);

                glGenBuffers(1, &vbo);
                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4, nullptr, GL_STATIC_DRAW);

                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, 0, 0);

                glBindVertexArray(0);
            }
        }

        void bind(GLint texture, const PostProcessingParams& post_params, size_t windowWidth) const
        {
            glUseProgram(program);

            // Vertex
            {
                // Shift the screen quad to account for the UI.
                float start = ((float(UI_WINDOW_WIDTH) / float(windowWidth)) * 2.0f) - 1.0f;
                glUniform1f(xStartLocation, start);
            }

            // Fragment
            {
                glUniform1i(displayTextureLocation, texture);
                glUniform1i(tonemappingEnabledLocation, post_params.tonemapping_enabled ? 1 : 0);
                glUniform1f(cameraExposureLocation, std::powf(2.0, post_params.exposure));
                glUniform1f(brightnessLocation, post_params.brightness);
                glUniform1f(contrastLocation, post_params.contrast);
                glUniform1f(hueLocation, post_params.hue);
                glUniform1f(saturationLocation, post_params.saturation);
                glUniform1f(vibranceLocation, post_params.vibrance);
                glUniform1f(redLocation, post_params.red);
                glUniform1f(greenLocation, post_params.green);
                glUniform1f(blueLocation, post_params.blue);
                glUniform1f(vignetteIntensityLocation, post_params.vignetteIntensity);
                glUniform1f(vignetteFalloffLocation, post_params.vignetteFalloff);
            }
        }

        void unbind() const
        {
            glUseProgram(0);
        }

        void draw(GLint texture, const PostProcessingParams& post_params, size_t windowWidth) const
        {
            bind(texture, post_params, windowWidth);

            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glBindVertexArray(0);
    
            unbind();
        }
    };

    PassGenerator m_renderer; // Generator of pathtraced frames.

    GLuint m_displayPixelBuffer = 0;  // PBO to use to put the pixels on the GPU.
    GLuint m_displayTexture     = 0;  // Texture which contains the final result for display.

    DisplayProgram m_displayProgram; // Used to display the final pixels to the screen.

    WindowParams m_windowParams; // Current size of the display window.
    WindowParams m_renderWindowParams; // Current size of the rendering window.

    std::atomic_flag m_shouldCopyPixels = false;   // If true, a new frame is available from the FrameGenerator.
    std::atomic<const float*> m_pathracedPixels = nullptr;  // Result of the pathtracer.
    glm::ivec2 m_pixelDimensions = glm::ivec2(0); // width,height of the pathtraced pixel data.
    bool m_justResized = false; // If true, the renderer has just processed a resize event.
    bool m_renderingFrame = false; // If true, the pathtracer is currently rendering a frame.
    bool m_resetRequested = true; // If true, a reset of the renderer has been requested.

    PassGenerator::RenderOptions m_renderOptions;

    size_t m_currentPass = 0;
    size_t m_totalPasses = 0;

    struct Camera
    {
        //FlyCamera flyCamera;
        OrbitCamera orbitCamera;
        bool locked = false;
    };

    Camera m_camera;
    bool m_visualizeSequenceData = false;
    std::vector<glm::vec2> m_sequenceVisualizationData;

    enum class SceneUnits {
        kMeters, // Heatray default.
        kCentimeters
    };
    SceneUnits m_sceneUnits = SceneUnits::kMeters;

    float m_currentPassTime = 0.0f;
    float m_totalRenderTime = 0.0f;

    std::string m_screenshotPath;
    bool m_hdrScreenshot = false;
    bool m_shouldSaveScreenshot = false;

    util::AABB m_sceneAABB;

    struct GroundPlane {
        size_t meshIndex = 0;
        bool exists = false;
    } m_groundPlane;

    bool m_cameraUpdated = false;
    float m_distanceScale = 1.0f;

    bool renderMaterialEditor(std::shared_ptr<Material> material);
    bool renderLightEditor(std::shared_ptr<Light> light);
    
    void updateCameraFromAABB();

    bool m_debugPassChanged = false;

    struct Editors {
        bool materialSelected = false;
        bool lightSelected = false;
    } m_editors;

    struct SceneTransform {
        float yaw = 0.0f;
        float pitch = 0.0f;
        float roll = 0.0f;
        float scale = 1.0f;
        
        glm::mat4 transform() const {
            return glm::scale(glm::mat4(1.0f), glm::vec3(scale)) *
                   glm::yawPitchRoll(yaw, pitch, roll);
        }
    } m_sceneTransform;
};
