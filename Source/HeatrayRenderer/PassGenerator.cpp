#include "PassGenerator.h"

#include <Utility/TextureLoader.h>

#include <RLWrapper/Error.h>
#include <RLWrapper/Shader.h>
#include <Utility/BlueNoise.h>
#include <Utility/FileIO.h>
#include <Utility/Random.h>
#include <Utility/ShaderCodeLoader.h>
#include <Utility/Timer.h>

#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

#include <assert.h>
#include <iostream>
#include <string>

PassGenerator::~PassGenerator()
{
    destroy();
}

void PassGenerator::init(const RLint renderWidth, const RLint renderHeight)
{
    // Fire up the render thread and push an init job to it to get going.
    m_jobThread = std::thread{ &PassGenerator::threadFunc, this };

    WindowSize size(renderWidth, renderHeight);
    Job job(JobType::kInit, std::make_any<WindowSize>(size));
    m_jobQueue.push(job);
}

void PassGenerator::destroy()
{
    if (m_jobThread.joinable())
    {
        Job job(JobType::kDestroy, std::make_any<void*>(nullptr));
        m_jobQueue.push(job);
        m_jobThread.join();
    }
}

void PassGenerator::resize(RLint newWidth, RLint newHeight)
{
    WindowSize size(newWidth, newHeight);
    Job job(JobType::kResize, std::make_any<WindowSize>(size));
    m_jobQueue.push(job);
}

void PassGenerator::renderPass(const RenderOptions& newOptions, PassCompleteCallback callback)
{
    m_passCompleteCallback = callback;

    RenderOptions options = newOptions;
    Job job(JobType::kRenderPass, std::make_any<RenderOptions>(options));
    m_jobQueue.push(job);
}

void PassGenerator::loadScene(LoadSceneCallback callback)
{
    m_loadSceneCallback = callback;

    Job job(JobType::kLoadScene, std::make_any<void *>(nullptr));
    m_jobQueue.push(job);
}

void PassGenerator::runOpenRLTask(OpenRLTask task)
{
	Job job(JobType::kGeneralTask, std::make_any<OpenRLTask>(task));
	m_jobQueue.push(job);
}

bool PassGenerator::runInitJob(const RLint renderWidth, const RLint renderHeight)
{
    // OpenRLContextAttribute attributes[] = {kOpenRL_EnableRayPrefixShaders, 1, NULL}; // For a future date.
    m_rlContext = OpenRLCreateContext(nullptr, nullptr, nullptr);
    RLFunc(OpenRLSetCurrentContext(m_rlContext));

    // Generate the lists of random sequences used for pathtracing.
    {
        generateRandomSequences(m_renderOptions.maxRenderPasses, m_renderOptions.sampleMode, m_renderOptions.bokehShape);
    }

    // Setup the buffers to render into.
    {
        m_fbo.create();

        openrl::Texture::Descriptor desc;
        desc.dataType = RL_FLOAT;
        desc.format = RL_RGBA;
        desc.internalFormat = RL_RGBA;
        desc.width = renderWidth;
        desc.height = renderHeight;

        openrl::Texture::Sampler sampler;
        sampler.minFilter = RL_NEAREST;
        sampler.magFilter = RL_NEAREST;

        m_fboTexture.create(nullptr, desc, sampler, false);
        m_fbo.addAttachment(m_fboTexture, RL_COLOR_ATTACHMENT0);

        assert(m_fbo.valid());

        // Set this framebuffers as the primary source to render into.
        m_fbo.bind();

        RLint bufferSize = renderWidth * renderHeight * sizeof(float) * openrl::PixelPackBuffer::kNumChannels;
        m_resultPixels.create(bufferSize);
    }

    // Load up the environment light primitive. This wil likely move to some future location.
    {
        openrl::Shader environmentLightShader;
        std::vector<std::string> shaderSource;
        util::loadShaderSourceFile("environmentLight.rlsl", shaderSource);
        if (!environmentLightShader.createFromMultipleStrings(shaderSource, RL_RAY_SHADER, "Environment Light Shader"))
        {
            return false;
        }

        openrl::Shader environmentLightVertexShader;
        shaderSource.clear();
        util::loadShaderSourceFile("passthrough.rlsl", shaderSource);
        if (!environmentLightVertexShader.createFromMultipleStrings(shaderSource, RL_VERTEX_SHADER, "Environment Light Vertex Shader"))
        {
            return false;
        }

        m_environmentLight.program.create();
		m_environmentLight.program.attach(environmentLightShader);
		m_environmentLight.program.attach(environmentLightVertexShader);
        if (!m_environmentLight.program.link("Environment Light Program"))
        {
            return false;
        }

		m_environmentLight.primitive.create();
		m_environmentLight.primitive.attachProgram(m_environmentLight.program);
		changeEnvironment(m_renderOptions.environment);
    }

    {
        // Setup global data buffer for all shaders.
        GlobalData data;
        data.environmentLight = m_environmentLight.primitive.primitive();
        m_globalData.load(&data, sizeof(GlobalData), "Global data buffer");
    }

    // Load the perspective camera frame shader for generating primary rays.
    {
        openrl::Shader frameShader;
        std::vector<std::string> shaderSource;
        util::loadShaderSourceFile("perspective.rlsl", shaderSource);
        if (!frameShader.createFromMultipleStrings(shaderSource, RL_FRAME_SHADER, "Perspective Frame Shader"))
        {
            return false;
        }

        m_frameProgram.create();
        m_frameProgram.attach(frameShader);
        if (!m_frameProgram.link("Perspecive Frame Shader"))
        {
            return false;
        }

        // Heatray uses the null primitive as the frame primitive.
        RLFunc(rlBindPrimitive(RL_PRIMITIVE, RL_NULL_PRIMITIVE));
        m_frameProgram.bind();
        m_frameProgram.setUniformBlock(m_frameProgram.getUniformBlockIndex("RandomSequences"), m_randomSequences.buffer());
        m_frameProgram.setUniformBlock(m_frameProgram.getUniformBlockIndex("Globals"), m_globalData.buffer());
    }

    return true;
}

void PassGenerator::runResizeJob(const RLint newRenderWidth, const RLint newRenderHeight)
{
    // Set the new viewport.
    rlViewport(0, 0, newRenderWidth, newRenderHeight);

    // Regenerate the render texture to be the proper size.
    if (m_fboTexture.valid())
    {
        m_fboTexture.resize(newRenderWidth, newRenderHeight);

        if (m_resultPixels.mapped())
        {
            m_resultPixels.unmapPixelData();
        }
        m_resultPixels.destroy();
        RLint bufferSize = newRenderWidth * newRenderHeight * sizeof(float) * openrl::PixelPackBuffer::kNumChannels;
        m_resultPixels.create(bufferSize);

        m_renderOptions.resetInternalState = true;
    }
}

void PassGenerator::runRenderFrameJob(const RenderOptions& newOptions)
{
    util::Timer timer(true);

    // The callback invoked at the end of this function _may_ map the pixel data and not unmap it before 
    // ticking this class again. In this case, unmap the pixel data at this point.
    if (m_resultPixels.mapped())
    {
        m_resultPixels.unmapPixelData();
    }

    if ((newOptions.enableInteractiveMode != m_renderOptions.enableInteractiveMode) || 
        (newOptions.resetInternalState != m_renderOptions.resetInternalState))
    {
        resetRenderingState(newOptions);
    }

    // Update global data for this frame.
    {
        m_globalData.bind();
        GlobalData* globalData = m_globalData.mapBuffer<GlobalData>();
        globalData->sampleIndex = m_currentSampleIndex;
        m_globalData.unmapBuffer(); 
        m_globalData.unbind(); 
    }
    
    glm::vec2 sensorDimensions(36.0f, 24.0f); // Dimensions of 35mm film.
    // https://en.wikipedia.org/wiki/Angle_of_view#Calculating_a_camera's_angle_of_view
    float fovY = 2.0f * std::atan2(sensorDimensions.y, 2.0f * m_renderOptions.camera.focalLength);

    m_frameProgram.bind();
    float fovTan = std::tanf(fovY * 0.5f);
    m_frameProgram.set1f(m_frameProgram.getUniformLocation("fovTan"), fovTan);
    m_frameProgram.set1f(m_frameProgram.getUniformLocation("aspectRatio"), m_renderOptions.camera.aspectRatio);
    m_frameProgram.set1f(m_frameProgram.getUniformLocation("focusDistance"), m_renderOptions.camera.focusDistance); 
    m_frameProgram.set1f(m_frameProgram.getUniformLocation("apertureRadius"), m_renderOptions.camera.apertureRadius);
    m_frameProgram.setMatrix4fv(m_frameProgram.getUniformLocation("viewMatrix"), &(m_renderOptions.camera.viewMatrix[0][0]));
    m_frameProgram.set2iv(m_frameProgram.getUniformLocation("blockSize"), &m_renderOptions.kInteractiveBlockSize.x);
    m_frameProgram.set2iv(m_frameProgram.getUniformLocation("currentBlockPixel"), &m_currentBlockPixel.x);
    m_frameProgram.set1i(m_frameProgram.getUniformLocation("interactiveMode"), m_renderOptions.enableInteractiveMode ? 1 : 0);
    m_frameProgram.setTexture(m_frameProgram.getUniformLocation("apertureSamplesTexture"), m_apertureSamplesTexture);

    if (m_renderOptions.enableInteractiveMode)
    {
        m_currentBlockPixel.x += 1;
        if (m_currentBlockPixel.x == m_renderOptions.kInteractiveBlockSize.x)
        {
            m_currentBlockPixel.x = 0;
            m_currentBlockPixel.y += 1;
            if (m_currentBlockPixel.y == m_renderOptions.kInteractiveBlockSize.y)
            {
                m_currentBlockPixel = glm::vec2(0, 0);
                ++m_currentSampleIndex;
            }
        }
    }
    else
    {
        ++m_currentSampleIndex;
    }

    RLFunc(rlRenderFrame());
    m_resultPixels.setPixelData(m_fboTexture);

    // Let the client know that a frame has been completed.
    float passTime = timer.stop();
    m_passCompleteCallback(m_resultPixels, passTime);
}

void PassGenerator::runLoadSceneJob()
{
    assert(m_loadSceneCallback);
    m_loadSceneCallback([this](const openrl::Program& program)
        {
            RLint sequenceIndex = program.getUniformBlockIndex("RandomSequences");
            if (sequenceIndex != -1)
            {
                program.setUniformBlock(sequenceIndex, m_randomSequences.buffer());
            }

            RLint globalsIndex = program.getUniformBlockIndex("Globals");
            if (globalsIndex != -1)
            {
                program.setUniformBlock(globalsIndex, m_globalData.buffer());
            }
        });
}

void PassGenerator::runDestroyJob()
{
    m_fbo.destroy();
    m_fboTexture.destroy();
    m_globalData.destroy();
    m_randomSequences.destroy();
    m_randomSequenceTexture.destroy();

    OpenRLDestroyContext(m_rlContext);
}

void PassGenerator::resetRenderingState(const RenderOptions& newOptions)
{
    m_currentSampleIndex = 0;
    m_currentBlockPixel = glm::ivec2(0, 0);
    rlClear(RL_COLOR_BUFFER_BIT);

    // Walk over the render options and switch things out iff something has changed.
    if ((m_renderOptions.environment.map != newOptions.environment.map) ||
		(m_renderOptions.environment.exposureCompensation != newOptions.environment.exposureCompensation) ||
		(m_renderOptions.environment.thetaRotation != newOptions.environment.thetaRotation)) {

		changeEnvironment(newOptions.environment);
    }

    if (m_renderOptions.sampleMode != newOptions.sampleMode ||
        m_renderOptions.maxRenderPasses != newOptions.maxRenderPasses ||
        m_renderOptions.bokehShape != newOptions.bokehShape) {

        generateRandomSequences(newOptions.maxRenderPasses, newOptions.sampleMode, newOptions.bokehShape);
    }

    if (m_renderOptions.maxRayDepth != newOptions.maxRayDepth) {
        m_globalData.bind();
        GlobalData* globalData = m_globalData.mapBuffer<GlobalData>();
        globalData->maxRayDepth = newOptions.maxRayDepth;
        m_globalData.unmapBuffer();
        m_globalData.unbind();
    }

    // Finally get all of the new render options.
    memcpy(&m_renderOptions, &newOptions, sizeof(RenderOptions));
    m_renderOptions.resetInternalState = false;
}

void PassGenerator::changeEnvironment(const RenderOptions::Environment &newEnv)
{
	if (newEnv.map != m_environmentLight.map_path) {
		m_environmentLight.map_path = newEnv.map;
		m_environmentLight.texture.destroy();
		if (newEnv.map != "white furnace test") {
			static const char* basePath = "Resources/Environments/";
			std::string fullPath = std::string(basePath) + newEnv.map;
			m_environmentLight.texture = util::loadTexture(fullPath.c_str(), true);
		} else {
			// Load a white furnace texture. Set to 0.8 instead of full white so that it's obvious if there is more energy being emitted
			// by the surface than should be.
			openrl::Texture::Descriptor desc;
			desc.dataType = RL_FLOAT;
			desc.format = RL_RGB;
			desc.internalFormat = RL_RGB;
			desc.width = 1;
			desc.height = 1;

			openrl::Texture::Sampler sampler;
			sampler.magFilter = RL_LINEAR;
			sampler.minFilter = RL_LINEAR;
			sampler.wrapS = RL_CLAMP_TO_EDGE;
			sampler.wrapT = RL_CLAMP_TO_EDGE;

			glm::vec3 data = glm::vec3(0.8f);
			m_environmentLight.texture.create(&data.x, desc, sampler, false);
		}
	}

	m_environmentLight.exposure_compensation = newEnv.exposureCompensation;
	m_environmentLight.thetaRotation = newEnv.thetaRotation;

	m_environmentLight.primitive.bind();
	m_environmentLight.program.bind();
	m_environmentLight.program.setTexture(m_environmentLight.program.getUniformLocation("environmentTexture"), m_environmentLight.texture);
	m_environmentLight.program.set1f(m_environmentLight.program.getUniformLocation("exposureCompensation"), std::powf(2.0f, m_environmentLight.exposure_compensation));
	m_environmentLight.program.set1f(m_environmentLight.program.getUniformLocation("thetaRotation"), m_environmentLight.thetaRotation);
	m_environmentLight.primitive.unbind();
}

void PassGenerator::generateRandomSequences(const RLint sampleCount, RenderOptions::SampleMode sampleMode, RenderOptions::BokehShape bokehShape)
{
    struct SequenceBlockData
    {
        RLtexture randomNumbers = RL_NULL_TEXTURE; ///< The actual sequence data stored in a 2D texture.
        RLfloat uvStep = 0.0f;  ///< The step to move forward to access the next sample (in UV space).
        RLfloat uvSequenceStep = 0.0f; ///< The step to move forward to access the next sequence (in UV space).
    };

    // If we already have previous sequence data, get rid of it.
    // Note that we don't delete the buffer however in order to keep all shader bindings valid.
    if (m_randomSequences.valid())
    {
        m_randomSequenceTexture.destroy();
    }
    else // This is the first time this function is being called.
    {
        m_randomSequences.setTarget(RL_UNIFORM_BLOCK_BUFFER);
        SequenceBlockData dummyData;
        m_randomSequences.load(&dummyData, sizeof(SequenceBlockData), "Random sequences uniform block");
    }

    // Note that each sequence is a 1D texture of values. Since OpenRL doesn't support 1D textures,
    // we make a 2D texture with a height of 1.
    openrl::Texture::Descriptor desc;
    desc.dataType       = RL_FLOAT;
    desc.format         = RL_RGB;
    desc.internalFormat = RL_RGB;
    desc.width          = sampleCount;
    desc.height         = kNumRandomSequences;

    openrl::Texture::Sampler sampler;
    sampler.minFilter = RL_NEAREST;
    sampler.magFilter = RL_NEAREST; 

    std::vector<glm::vec3> values(kNumRandomSequences * sampleCount);
    for (unsigned int iSequence = 0; iSequence < kNumRandomSequences; ++iSequence)
    {
        switch (sampleMode)
        {
            case RenderOptions::SampleMode::kRandom:
                util::uniformRandomFloats<glm::vec3>(&values[iSequence * sampleCount], sampleCount, iSequence, 0.0f, 1.0f);
                break;
            case RenderOptions::SampleMode::kHalton:
                util::halton(&values[iSequence * sampleCount], sampleCount, iSequence);
                break;
            case RenderOptions::SampleMode::kHammersley:
                util::hammersley(&values[iSequence * sampleCount], sampleCount, iSequence);
                break;
            case RenderOptions::SampleMode::kBlueNoise:
            {
                util::blueNoise(&values[iSequence * sampleCount], sampleCount, iSequence);
                break;
            }
            default:
                assert("Unknown sample mode specified");
        }
    }

    // Load the data into a 2D texture and associate it with the overall sequence data.
    // The sequence data will be stored in a 2D texture that has the following layout:
    //      width  - number of samples in the sequence
    //      height -kNumRandomSequences -- therefore each row is a unique set of sequence data.
    m_randomSequenceTexture.create(&values[0], desc, sampler, false);

    m_randomSequences.bind();
    SequenceBlockData* sequences = m_randomSequences.mapBuffer<SequenceBlockData>(RL_WRITE_ONLY);
    sequences->randomNumbers = m_randomSequenceTexture.texture();
    sequences->uvStep = 1.0f / static_cast<RLfloat>(sampleCount);
    sequences->uvSequenceStep = 1.0f / static_cast<RLfloat>(kNumRandomSequences);
    m_randomSequences.unmapBuffer();
    m_randomSequences.unbind();

    // Now generate the data random sequence data for aperture sampling for depth of field.
    {
        std::vector<glm::vec3> values(kNumRandomSequences * sampleCount);
        for (unsigned int iSequence = 0; iSequence < kNumRandomSequences; ++iSequence)
        {
            //util::radialPseudoRandom(&values[iSequence * sampleCount], sampleCount, iSequence);
            util::randomPolygonal(&values[iSequence * sampleCount], 5, sampleCount, iSequence);

            switch (bokehShape)
            {
                case PassGenerator::RenderOptions::BokehShape::kSpherical:
                    util::radialPseudoRandom(&values[iSequence * sampleCount], sampleCount, iSequence);
                    break;
                case PassGenerator::RenderOptions::BokehShape::kPentagon:
                    util::randomPolygonal(&values[iSequence * sampleCount], 5, sampleCount, iSequence);
                    break;
                case PassGenerator::RenderOptions::BokehShape::kHexagon:
                    util::randomPolygonal(&values[iSequence * sampleCount], 6, sampleCount, iSequence);
                    break;
                case PassGenerator::RenderOptions::BokehShape::kOctagon:
                    util::randomPolygonal(&values[iSequence * sampleCount], 8, sampleCount, iSequence);
                    break;
                default:
                    assert(0);
            }
        }

        if (m_apertureSamplesTexture.valid())
        {
            m_apertureSamplesTexture.destroy();
        }

        m_apertureSamplesTexture.create(&values[0], desc, sampler, false);
    }
 }

void PassGenerator::threadFunc()
{
    // This is the core of the raytracing loop. Commands are processed here until a kDestroy command
    // has been received, at which point this thread will exit.
    while (true)
    {
        if (m_jobQueue.size())
        {
            Job job = m_jobQueue.front();
            m_jobQueue.pop();

            switch (job.type)
            {
                case JobType::kInit:
                {
                    WindowSize size = std::any_cast<WindowSize>(job.params);
                    if (!runInitJob(size.width, size.height))
                    {
                        return; // End the thread.
                    }
                    break;
                }
                case JobType::kResize:
                {
                    WindowSize size = std::any_cast<WindowSize>(job.params);
                    runResizeJob(size.width, size.height);
                    break;
                }
                case JobType::kRenderPass:
                {
                    RenderOptions options = std::any_cast<RenderOptions>(job.params);
                    runRenderFrameJob(options);
                    break;
                }
                case JobType::kLoadScene:
                {
                    runLoadSceneJob();
                    break;
                }
                case JobType::kDestroy:
                {
                    runDestroyJob();
                    return; // End the thread.
                    break;
                }
				case JobType::kGeneralTask:
				{
					OpenRLTask task = std::any_cast<OpenRLTask>(job.params);
					task();
					break;
				}

                default:
                    assert(0 && "Invalid job type");
                    break;
            }
        }
    }
}



