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
#include "Materials/PhysicallyBasedMaterial.h"

#include <Utility/FileIO.h>
#include <Utility/AABB.h>

#include <atomic>
#include <memory>

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

	static constexpr size_t UI_WINDOW_WIDTH = 500;

private:
    // This class is NOT copyable (move is ok).
    HeatrayRenderer(const HeatrayRenderer& other) = delete;
    HeatrayRenderer& operator=(const HeatrayRenderer& other) = delete;

    void resizeGLData();
    void generateSequenceVisualizationData(int sequenceIndex, int renderPasses);
    bool renderUI();
    void resetRenderer();
    void saveScreenshot();

	struct PostProcessingParams {
		bool tonemapping_enabled = false;
		float exposure = 0.0f;
		float brightness = 0.0f;
		float contrast = 0.0f;
		float hue = 1.0f;
		float saturation = 1.0f;
		float vibrance = 0.0f;
		float red = 1.0f;
		float green = 1.0f;
		float blue = 1.0f;
	} m_post_processing_params;

    /// Object which handles final display of the raytraced pixels via a custom fragment shader.
    struct DisplayProgram
    {
        GLuint program = 0;
        GLuint shader = 0;

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
			if (success != GL_TRUE) {
				GLsizei log_length = 0;
				GLchar  log[1024];
				glGetShaderInfoLog(shader, sizeof(log), &log_length, log);
				std::cout << "Unable to compile display shader: " << std::endl << log << std::endl;
			}
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
			brightnessLocation = glGetUniformLocation(program, "brightness");
			contrastLocation = glGetUniformLocation(program, "contrast");
			hueLocation = glGetUniformLocation(program, "hue");
			saturationLocation = glGetUniformLocation(program, "saturation");
			vibranceLocation = glGetUniformLocation(program, "vibrance");
			redLocation = glGetUniformLocation(program, "red");
			greenLocation = glGetUniformLocation(program, "green");
			blueLocation = glGetUniformLocation(program, "blue");
        }

        /// @param texture Location of the texture to use for the shader.
		void bind(GLint texture, const PostProcessingParams& post_params) const
        {
            glUseProgram(program);
            glUniform1i(displayTextureLocation, texture);
            glUniform1i(tonemappingEnabledLocation, post_params.tonemapping_enabled ? 1 : 0);
            glUniform1f(cameraExposureLocation, std::powf(2.0, post_params.exposure));
			glUniform1f(brightnessLocation, post_params.brightness);
			glUniform1f(hueLocation, post_params.hue);
			glUniform1f(saturationLocation, post_params.saturation);
			glUniform1f(vibranceLocation, post_params.vibrance);
			glUniform1f(redLocation, post_params.red);
			glUniform1f(greenLocation, post_params.green);
			glUniform1f(blueLocation, post_params.blue);

			{
				// Contrast needs to be properly remapped before being used by the shader.
				float integer_contrast = post_params.contrast * 255.0f;
				float contrast = (259.0f * (integer_contrast + 255.0f)) / (255.0f * (259.0f - integer_contrast));
				glUniform1f(contrastLocation, contrast);
			}
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
	WindowParams m_renderWindowParams; ///< Current size of the rendering window.

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
    bool m_tonemappingEnabled = false;

	enum class SceneUnits {
		kMeters, // Heatray default.
		kCentimeters
	};
	SceneUnits m_scene_units = SceneUnits::kMeters;

    std::vector<RLMesh> m_sceneData;

    bool m_userRequestedFileLoad = false;
    bool m_swapYZ = false;

    float m_currentPassTime = 0.0f;
    float m_totalRenderTime = 0.0f;

	std::string m_screenshotPath;
    bool m_hdrScreenshot = false;
    bool m_shouldSaveScreenshot = false;

	util::AABB m_sceneAABB;

	struct GroundPlane {
		void reset() {
			if (mesh) {
				material.reset();
				mesh = nullptr;
				meshIndex = ~0;
				materialParams = PhysicallyBasedMaterial::Parameters();
			}
		}

		RLMesh* mesh = nullptr;
		std::shared_ptr<PhysicallyBasedMaterial> material = nullptr;
		size_t meshIndex = ~0;
		PhysicallyBasedMaterial::Parameters materialParams;
	} m_groundPlane;
};