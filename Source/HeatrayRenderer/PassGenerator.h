//
//  PassGenerator.h
//  Heatray
//
//  Responsible for generating a single pass of the pathtraced data.
//
//

#pragma once

#include "RLMesh.h"

#include <RLWrapper/Framebuffer.h>
#include <RLWrapper/PixelPackBuffer.h>
#include <RLWrapper/Primitive.h>
#include <RLWrapper/Program.h>
#include <RLWrapper/Texture.h>
#include <Utility/AsyncTaskQueue.h>

#include <glm/glm/mat4x4.hpp>
#include <OpenRL/OpenRL.h>
#include <any>
#include <functional>
#include <memory>
#include <queue>
#include <thread>
#include <vector>

struct CameraInfo
{
    float fov = 0.0f; // Vertical FOV (in radians).

    // In the future: aperature, focal distance, etc.
};
   
class PassGenerator              
{
public:
    PassGenerator() = default;         
    ~PassGenerator();     
    
    /// Initialize HeatRay and get the pathtracer ready for use.
    /// Must be called prior to doing any rendering.
    /// @param renderWidth Width to initialize the renderer to (in pixels).
    /// @param renderHeight Height to initialize the renderer to (in pixels).
    /// @returns If true, the path tracer is ready for use.
    void init(const RLint renderWidth, const RLint renderHeight);

    struct RenderOptions {
        // If true, the renderer will only do one pixel within a given block each frame as opposed to all of the pixels.
        // This speeds up raytraced for a more interactive experience but is slower to generate final frames.
        // Defaulted on.
        bool enableInteractiveMode = true;
        static constexpr glm::ivec2 kInteractiveBlockSize = glm::ivec2(5, 5);

        bool resetInternalState = true; 
        unsigned int maxRenderPasses = 32; 

		struct Environment {
			std::string map = "studio.hdr";
			bool builtInMap = true;
			float exposureCompensation = 0.0f;
			float thetaRotation = 0.0f; // Extra rotation to apply to the environment map.
		} environment;

        std::string scene = "Multi-Material";

        struct Camera {
			static constexpr size_t NUM_FSTOPS = 11;
			static constexpr float fstopOptions[NUM_FSTOPS] = {
				32.0f, 22.0f, 16.0f, 11.0f, 8.0f, 5.6f, 4.0f, 2.8f, 2.0f, 1.4f, 1.0f
			};

            float aspectRatio    = -1.0f;  ///< Width / height.
            float focusDistance  = 1.0f;   ///< In meters.
            float focalLength    = 50.0f;  ///< In millimeters.
            float apertureRadius = 0.0f;   ///< In meters (should not be set manually!).
			float fstop			 = fstopOptions[0];
            glm::mat4 viewMatrix = glm::mat4(1.0f);

			void setApertureRadius() {
				apertureRadius = (focalLength / fstop) / 1000.0f; // Value is in meters.
			}
        } camera;

        enum class SampleMode {
            kRandom,     ///< Perform pseudo-random sampling using uniform distributions.
            kHalton,     ///< Perform sampling using a Halton sequence.
            kHammersley, ///< Perform sampling using a Hammersley sequence.
            kBlueNoise,
			kSobol
        };

        SampleMode sampleMode = SampleMode::kSobol;

        enum class BokehShape {
			kCircular,
            kPentagon,
            kHexagon,
            kOctagon
        };

        BokehShape bokehShape = BokehShape::kCircular;

        int maxRayDepth = 10;
    };
    
    ///
    /// Deallocate any internal data and
    /// prepare for shutdown.
    ///
    void destroy();

    using PassCompleteCallback = std::function<void(const openrl::PixelPackBuffer& resultPixels, float passTime)>;
    void renderPass(const RenderOptions& newOptions, PassCompleteCallback callback);
    
    ///
    /// Resize the raytracer output.
    ///
    /// @param newWidth New width of the raytraced output buffer (in pixels).
    /// @param newHeight New height of the raytraced output buffer (in pixels).
    ///
    void resize(const RLint newWidth, const RLint newHeight);

    using LoadSceneCallback = std::function<void(RLMesh::SetupSystemBindingsCallback systemSetupCallback)>;
    void loadScene(LoadSceneCallback callback);

	///
	/// Run a general OpenRL task. This can be useful for things that need to happen on the OpenRL thread
	/// but don't need a dedicated job.
	///
	using OpenRLTask = std::function<void()>;
	void runOpenRLTask(OpenRLTask task);

    static constexpr RLint kNumRandomSequences = 16;
    
private:
    
    // This class is not copyable.
    PassGenerator(const PassGenerator& other) = delete;
    PassGenerator(const PassGenerator&& other) = delete;
    PassGenerator& operator=(const PassGenerator& other) = delete;
    PassGenerator& operator=(const PassGenerator&& other) = delete;

    void changeEnvironment(const RenderOptions::Environment& newEnv);
    void generateRandomSequences(const RLint sampleCount, RenderOptions::SampleMode sampleMode, RenderOptions::BokehShape bokehShape);
    void resetRenderingState(const RenderOptions& newOptions);

    bool runInitJob(const RLint renderWidth, const RLint renderHeight);
    void runResizeJob(const RLint newRenderWidth, const RLint newRenderHeight);
    void runRenderFrameJob(const RenderOptions& newOptions);
    void runLoadSceneJob();
    void runDestroyJob();
        
    OpenRLContext m_rlContext = nullptr;

    std::shared_ptr<openrl::Framebuffer> m_fbo = nullptr;          ///< Framebuffer object used for rendering to.
	std::shared_ptr <openrl::Texture>    m_fboTexture = nullptr;   ///< Color buffer attached to the main framebuffer object.

	std::shared_ptr <openrl::Program> m_frameProgram = nullptr; ///< Current frame program used for generating primary rays.

    openrl::PixelPackBuffer m_resultPixels;

	struct EnvironmentLight {
		std::shared_ptr<openrl::Primitive> primitive = nullptr;
		std::shared_ptr<openrl::Program> program = nullptr;
		std::shared_ptr<openrl::Texture> texture = nullptr;
		float exposure_compensation = 0.0f;
		float thetaRotation = 0.0f;
		std::string map_path;

		void reset() {
			primitive.reset();
			program.reset();
			texture.reset();
		}
	} m_environmentLight;

    PassCompleteCallback m_passCompleteCallback;
    LoadSceneCallback m_loadSceneCallback;
    unsigned int m_currentSampleIndex = 0;

    enum class JobType {
        kInit,
        kResize,
        kRenderPass,
        kLoadScene,
        kDestroy,
		kGeneralTask
    };

    struct Job {
        Job(const JobType t, std::any p) : type(t), params(std::move(p)) {}

        JobType type;
        std::any params;
    };

    struct WindowSize {
        WindowSize(RLint newWidth, RLint newHeight) : width(newWidth), height(newHeight) {}
        RLint width  = -1;
        RLint height = -1;
    };

	util::AsyncTaskQueue<Job> m_jobProcessor; ///< Used to process all jobs on the render thread.

    RenderOptions m_renderOptions;

    glm::ivec2 m_currentBlockPixel = glm::ivec2(0, 0);

	std::shared_ptr < openrl::Buffer>  m_randomSequences = nullptr;
	std::shared_ptr < openrl::Texture> m_randomSequenceTexture = nullptr; ///< Series of random sequences stored in a 2D texture. Each row of the texture is a different sequence.
	std::shared_ptr < openrl::Texture> m_apertureSamplesTexture = nullptr; ///< Randomly generated values to use while sampling the aperture for depth of field.

    struct GlobalData {
        int maxRayDepth = 5;
        int sampleIndex = 0;
        RLprimitive environmentLight = RL_NULL_PRIMITIVE;
    };
	std::shared_ptr<openrl::Buffer> m_globalData = nullptr;
};
