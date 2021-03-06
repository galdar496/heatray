//
//  HeatrayRenderer.h
//  Heatray
//
//  This is the main API to the pathtracer including scene loading, shader creation,
//  and rendering. This connects both the OpenRL-based path tracer with the OpenGL-based
//  frame visualization.
//

#pragma once

#if defined(_WIN32) || defined(_WIN64)
    #include <glew/GL/glew.h>
    #include <gl/GL.h>
#else
    #include <OpenGL/gl.h>
#endif

#include "FlyCamera.h"
#include "OrbitCamera.h"
#include "PassGenerator.h"
#include "RLMesh.h"

#include <Utility/FileIO.h>

#include <atomic>

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
        GLint width    = -1;
        GLint height   = -1;
    };

    bool init(const GLint renderWidth, const GLint renderHeight);
    void destroy();
    void resize(const GLint newWidth, const GLint newHeight);
    void changeScene(std::string const & sceneName);
    void render();
    void handlePendingFileLoads();

private:
    // This class is NOT copyable (move is ok).
    HeatrayRenderer(const HeatrayRenderer& other) = delete;
    HeatrayRenderer& operator=(const HeatrayRenderer& other) = delete;

    void resizeGLData();
    void generateSequenceVisualizationData(int sequenceIndex, int renderPasses);
    bool renderUI();
    void resetRenderer();
    void saveScreenshot();

    /// Object which handles final display of the raytraced pixels via a custom fragment shader.
    struct DisplayProgram
    {
        GLuint program = 0;
        GLuint shader = 0;

        GLint displayTextureLocation = -1;
        GLint tonemappingEnabledLocation = -1;
        GLint cameraExposureLocation = -1;

        void init()
        {
            shader = glCreateShader(GL_FRAGMENT_SHADER);
            std::string shaderSource;
            util::readTextFile("Resources/Shaders/DisplayGL.frag", shaderSource);
            const char* source = shaderSource.c_str();
            glShaderSource(shader, 1, &source, nullptr);
            glCompileShader(shader);
            GLint success = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            assert(success == GL_TRUE);

            // Create the display program.
            program = glCreateProgram();
            glAttachShader(program, shader);
            glLinkProgram(program);
            glGetProgramiv(program, GL_LINK_STATUS, &success);
            assert(success == GL_TRUE);

            // Read the necessary uniform locations.
            displayTextureLocation = glGetUniformLocation(program, "raytracedTexture");
            tonemappingEnabledLocation = glGetUniformLocation(program, "tonemappingEnabled");
            cameraExposureLocation = glGetUniformLocation(program, "cameraExposure");
        }

        /// @param texture Location of the texture to use for the shader.
        void bind(GLint texture, bool tonemappingEnabled, float cameraExposure) const
        {
            glUseProgram(program);
            glUniform1i(displayTextureLocation, texture);
            glUniform1i(tonemappingEnabledLocation, tonemappingEnabled ? 1 : 0);
            glUniform1f(cameraExposureLocation, std::powf(2.0, cameraExposure));
        }

        void unbind() const
        {
            glUseProgram(0);
        }
    };

    PassGenerator m_renderer; ///< Generator of pathtraced frames.

    GLuint m_displayPixelBuffer = 0;  ///< PBO to use to put the pixels on the GPU.
    GLuint m_displayTexture     = 0;  ///< Texture which contains the final result for display.

    DisplayProgram m_displayProgram; ///< Used to display the final pixels to the screen.

    WindowParams m_windowParams; ///< Current size of the display window.

    std::atomic<bool> m_shouldCopyPixels = false;   ///< If true, a new frame is available from the FrameGenerator. 
    std::atomic<const float*> m_pathracedPixels = nullptr;  ///< Result of the pathtracer.
    glm::ivec2 m_pixelDimensions; ///< width,height of the pathtraced pixel data.
    bool m_justResized = false; ///< If true, the renderer has just processed a resize event.
    bool m_renderingFrame = false; ///< If true, the pathtracer is currently rendering a frame.
    bool m_resetRequested = true; ///< If true, a reset of the renderer has been requested.

    PassGenerator::RenderOptions m_renderOptions;

    size_t m_currentPass = 0;
    size_t m_totalPasses = 0;

    struct Camera
    {
        FlyCamera flyCamera;
        OrbitCamera orbitCamera;
    };

    Camera m_camera;
    bool m_visualizeSequenceData = false;
    std::vector<glm::vec3> m_sequenceVisualizationData;

    float m_cameraExposure = 0.0f;
    bool m_tonemappingEnabled = true;

    std::vector<RLMesh> m_sceneData;

    bool m_userRequestedFileLoad = false;
    bool m_swapYZ = false;

    float m_currentPassTime = 0.0f;
    float m_totalRenderTime = 0.0f;

    static const size_t m_screenshotPathLength = 512;
    char m_screenshotPath[m_screenshotPathLength] = { 0 };
    bool m_hdrScreenshot = false;
    bool m_shouldSaveScreenshot = false;

};