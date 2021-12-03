//
//  PassGenerator.h
//  Heatray
//
//  Responsible for generating a single pass (sample) of the pathtraced data.
//
//

#pragma once

#include <Utility/AsyncTaskQueue.h>

#include <glm/glm/mat4x4.hpp>
#include <glm/glm/ext/scalar_constants.hpp>
#include <OpenRL/OpenRL.h>

#include <any>
#include <functional>
#include <memory>
#include <string>
#include <vector>

// Forward declarations.
namespace openrl {
    class Buffer;
    class Framebuffer;
    class PixelPackBuffer;
    class Program;
    class Texture;
} // namespace openrl.
class EnvironmentLight;
class Lighting;
class Scene;
   
class PassGenerator              
{
public:
    PassGenerator() = default;         
    ~PassGenerator();     
    
    //-------------------------------------------------------------------------
    // Initialize HeatRay and get the pathtracer ready for use. Must be called 
    // prior to doing any rendering. Parameters are in pixels.
    void init(const RLint renderWidth, const RLint renderHeight);

    //-------------------------------------------------------------------------
    // These options control the current state of the renderer are are used
    // when resetting the renderer.
    struct RenderOptions {
        // If true, the renderer will only do one pixel within a given block each frame as opposed to all of the pixels.
        // This speeds up raytraced for a more interactive experience but is slower to generate final frames.
        // Defaulted on.
        bool enableInteractiveMode = true;
        static constexpr glm::ivec2 kInteractiveBlockSize = glm::ivec2(3, 3);

        bool resetInternalState = true; 
        uint32_t maxRenderPasses = 32;
        uint32_t maxRayDepth = 10; // How many bounces a ray can make before being terminated.
        float maxChannelValue = glm::pi<float>();

        std::string scene; // Name of the scene.

        struct Environment {
            std::string map; // Name of the IBL to load.
            glm::vec3 solidColor = glm::vec3(0.5f); // Optional solid color that can be set for the environment map.
            bool builtInMap = true; // If true, we're loading one of Heatray's built-in IBLs.
            float exposureCompensation = 0.0f;
            float thetaRotation = 0.0f; // Extra rotation to apply to the environment map.
        } environment;

        struct Camera {
            static constexpr size_t NUM_FSTOPS = 12;
            static constexpr float fstopOptions[NUM_FSTOPS] = {
                std::numeric_limits<float>::max(), 32.0f, 22.0f, 16.0f, 11.0f, 8.0f, 5.6f, 4.0f, 2.8f, 2.0f, 1.4f, 1.0f
            };

            float aspectRatio    = -1.0f;  // Width / height.
            float focusDistance  = 1.0f;   // In meters.
            float focalLength    = 50.0f;  // In millimeters.
            float apertureRadius = 0.0f;   // In meters (should not be set manually!).
            float fstop			 = fstopOptions[1];
            glm::mat4 viewMatrix = glm::mat4(1.0f);

            //-------------------------------------------------------------------------
            // Should be called to update the internal aperture radius based off various
            // camera parameters.
            void setApertureRadius() {
                apertureRadius = (focalLength / fstop) / 1000.0f; // Value is in meters.
            }
        } camera;

        //-------------------------------------------------------------------------
        // Controls which sampling algorithm is employed for the generation of
        // random numbers.
        enum class SampleMode {
            kRandom,     // Perform pseudo-random sampling using a uniform distribution.
            kHalton,     // Perform sampling using a Halton sequence.
            kHammersley, // Perform sampling using a Hammersley sequence.
            kBlueNoise,  // Perform sampling using a Blue Noise sequence.
            kSobol       // Perform sampling using the Sobol sequence.
        };

        SampleMode sampleMode = SampleMode::kSobol;

        //-------------------------------------------------------------------------
        // Controls the Bokeh shape generated during depth of field.
        enum class BokehShape {
            kCircular,
            kPentagon,
            kHexagon,
            kOctagon
        };

        BokehShape bokehShape = BokehShape::kCircular;

        //-------------------------------------------------------------------------
        // Used to visualize various mesh parameters.
        enum class DebugVisualizationMode {
            kNone,
            kGeometricNormals,
            kUVs,
            kTangents,
            kBitangents,
            kNormalmap,
            kFinalNormals,
            kBaseColor,
            kRoughness,
            kMetallic,
            kEmissive,
            kClearcoat,
            kClearcoatRoughness,
            kClearcoatNormalmap,
            kShader
        };

        DebugVisualizationMode debugVisMode = DebugVisualizationMode::kNone;

        // If true, the renderer does not continually generate new frames. This allows
        // the user to specify an index between [0 - maxNumPasses] and see what the
        // result is for that single pass.
        bool debugPassRendering = false;
        int debugPassIndex = 0;
    };
    
    //-------------------------------------------------------------------------
    // Deallocate any internal data and prepare for shutdown.
    void destroy();

    //-------------------------------------------------------------------------
    // Render a pass (sample) of the path tracer. If new options are passed 
    // via RenderOptions to this function then the renderer is reset prior to
    // rendering this pass. Upon completion the PassCompleteCallback will be
    // invoked.
    using PassCompleteCallback = std::function<void(std::shared_ptr<openrl::PixelPackBuffer> resultPixels, float passTime)>;
    void renderPass(const RenderOptions& newOptions, PassCompleteCallback callback);
    
    //-------------------------------------------------------------------------
    // Resize the pathtracer output. Parameters are in pixels.
    void resize(const RLint newWidth, const RLint newHeight);

    //-------------------------------------------------------------------------
    // Load new scene data via a user-supplied callback. If 'clearOldScene' is 
    // true then all internal scene data will be cleared prior to invoking the
    // user callback.
    using LoadSceneCallback = std::function<void(std::shared_ptr<Scene> scene)>;
    void loadScene(LoadSceneCallback callback, bool clearOldScene = true);

    //-------------------------------------------------------------------------
    // Change the scene lighting via a user-supplied callback.
    using LightingCallback = std::function<void(std::shared_ptr<Lighting> lighting)>;
    void changeLighting(LightingCallback callback);

    //-------------------------------------------------------------------------
    // Run a general OpenRL task. This can be useful for things that need to 
    // happen on the OpenRL thread but don't need a dedicated job.
    using OpenRLTask = std::function<void()>;
    void runOpenRLTask(OpenRLTask task);

    std::shared_ptr<Scene> scene() const { return m_scene; }

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
    void runLoadSceneJob(bool clearOldScene);
    void runDestroyJob();
        
    OpenRLContext m_rlContext = nullptr;

    std::shared_ptr<openrl::Framebuffer> m_fbo = nullptr;          // Framebuffer object used for rendering to.
    std::shared_ptr <openrl::Texture>    m_fboTexture = nullptr;   // Color buffer attached to the main framebuffer object.

    std::shared_ptr <openrl::Program> m_frameProgram = nullptr; // Current frame program used for generating primary rays.

    std::shared_ptr<openrl::PixelPackBuffer> m_resultPixels = nullptr; // Pixels from all previous passes since the last framebuffer clear.

    std::shared_ptr<EnvironmentLight> m_environmentLight = nullptr;

    PassCompleteCallback m_passCompleteCallback;
    LoadSceneCallback m_loadSceneCallback;
    unsigned int m_currentSampleIndex = 0;

    //-------------------------------------------------------------------------
    // Job types supported by the internal job system. Since all OpenRL calls
    // must happen on the same thread, all public functions for this class
    // create jobs and push them to the internal OpenRL thread.
    enum class JobType {
        kInit,
        kResize,
        kRenderPass,
        kLoadScene,
        kChangeLighting,
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

    util::AsyncTaskQueue<Job> m_jobProcessor; // Used to process all jobs on the OpenRL thread.

    RenderOptions m_renderOptions;

    glm::ivec2 m_currentBlockPixelSample = glm::ivec2(0, 0);

    std::shared_ptr<openrl::Buffer>  m_randomSequences = nullptr;
    std::shared_ptr<openrl::Texture> m_randomSequenceTexture = nullptr;   // Series of random sequences stored in a 2D texture. Each row of the texture is a different sequence.
    std::shared_ptr<openrl::Texture> m_apertureSamplesTexture = nullptr;  // Randomly generated values to use while sampling the aperture for depth of field.
    std::shared_ptr<openrl::Texture> m_sequenceOffsetsTexture = nullptr;  // Randomly generated values to use while determining which sample to start a camera ray with.

    //-------------------------------------------------------------------------
    // Global data shared by all OpenRL shaders.
    struct GlobalData {
        int maxRayDepth = 5;
        int sampleIndex = 0;
        float maxChannelValue = glm::pi<float>();

        // Debugging.
        int enableDebugVisualizer = 0;
        int showGeometricNormals = 0;
        int showUVs = 0;
        int showTangents = 0;
        int showBitangents = 0;
        int showNormalmap = 0;
        int showFinalNormals = 0;
        int showBaseColor = 0;
        int showRoughness = 0;
        int showMetallic = 0;
        int showEmissive = 0;
        int showClearcoat = 0;
        int showClearcoatRoughness = 0;
        int showClearcoatNormalmap = 0;
        int showShader = 0;
    };
    std::shared_ptr<openrl::Buffer> m_globalData = nullptr;

    std::shared_ptr<Scene> m_scene = nullptr; // All loaded scene data.

    // 2D pixel coordinates used when determine which pixel within a block
    // should sample when in interactive mode.
    std::shared_ptr<openrl::Texture> m_interactiveBlockCoordsTexture = nullptr;
};
