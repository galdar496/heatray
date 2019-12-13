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

#include <glm/glm/mat4x4.hpp>
#include <OpenRL/OpenRL.h>
#include <any>
#include <functional>
#include <queue>
#include <thread>
#include <vector>

struct CameraInfo
{
    float fov; // Vertical FOV (in radians).

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

    struct RenderOptions
    {
        // If true, the renderer will only do one pixel within a given block each frame as opposed to all of the pixels.
        // This speeds up raytraced for a more interactive experience but is slower to generate final frames.
        // Defaulted on.
        bool enableInteractiveMode = true;
        static constexpr glm::ivec2 kInteractiveBlockSize = glm::ivec2(5, 5);

        bool resetInternalState = true; 
        unsigned int maxRenderPasses = 32; 
        std::string environmentMap = "arches.hdr";

        std::string scene = "Multi-Material";

        struct Camera
        {
            float aspectRatio = -1.0f;  ///< Width / height.
            float focusDistance = 1.0f; ///< In meters.
            float focalLength = 50.0f;  ///< in millimeters.
            float fstop = 22.0f;        ///< f-stop of the camera.
            glm::mat4 viewMatrix = glm::mat4(1.0f);
        } camera;

        enum class SampleMode
        {
            kRandom,     ///< Perform pseudo-random sampling using uniform distributions.
            kHalton,     ///< Perform sampling using a Halton sequence.
            kHammersley, ///< Perform sampling using a Hammersley sequence.
            kBlueNoise,
        };

        SampleMode sampleMode = SampleMode::kHalton;

        int maxRayDepth = 5;
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

    static constexpr RLint kNumRandomSequences = 16;
    
private:
    
    // This class is not copyable.
    PassGenerator(const PassGenerator& other) = delete;
    PassGenerator(const PassGenerator&& other) = delete;
    PassGenerator& operator=(const PassGenerator& other) = delete;
    PassGenerator& operator=(const PassGenerator&& other) = delete;

    void changeEnvironment(std::string const & newEnvMap);
    void generateRandomSequences(const RLint sampleCount, RenderOptions::SampleMode sampleMode);
    void resetRenderingState(const RenderOptions& newOptions);

    bool runInitJob(const RLint renderWidth, const RLint renderHeight);
    void runResizeJob(const RLint newRenderWidth, const RLint newRenderHeight);
    void runRenderFrameJob(const RenderOptions& newOptions);
    void runLoadSceneJob();
    void runDestroyJob();
        
    OpenRLContext m_rlContext = nullptr;

    openrl::Framebuffer m_fbo;          ///< Framebuffer object used for rendering to.
    openrl::Texture     m_fboTexture;   ///< Color buffer attached to the main framebuffer object.

    openrl::Program m_frameProgram; ///< Current frame program used for generating primary rays.

    openrl::PixelPackBuffer m_resultPixels;

    openrl::Primitive m_environmentLightPrimitive;
    openrl::Program m_environmentLightProgram;
    openrl::Texture m_environmentTexture;

    PassCompleteCallback m_passCompleteCallback;
    LoadSceneCallback m_loadSceneCallback;
    unsigned int m_currentSampleIndex = 0;

    enum class JobType
    {
        kInit,
        kResize,
        kRenderPass,
        kLoadScene,
        kDestroy
    };

    struct Job
    {
        Job(const JobType t, std::any p) : type(t), params(std::move(p)) {}

        JobType type;
        std::any params;
    };

    struct WindowSize
    {
        WindowSize(RLint newWidth, RLint newHeight) : width(newWidth), height(newHeight) {}
        RLint width  = -1;
        RLint height = -1;
    };

    std::queue<Job> m_jobQueue; ///< List of jobs to process on the render thread.
    std::thread m_jobThread;

    void threadFunc();

    RenderOptions m_renderOptions;

    glm::ivec2 m_currentBlockPixel = glm::ivec2(0, 0);

    openrl::Buffer m_randomSequences;
    openrl::Texture m_randomSequenceTexture; ///< Series of random sequences stored in a 2D texture. Each row of the texture is a different sequence.
    openrl::Texture m_apertureSamplesTexture; ///< Randomly generated values to use while sampling the aperture for depth of field.

    struct GlobalData
    {
        int maxRayDepth = 5;
        int sampleIndex = 0;
        RLprimitive environmentLight = RL_NULL_PRIMITIVE;
    };
    openrl::Buffer m_globalData;
};
