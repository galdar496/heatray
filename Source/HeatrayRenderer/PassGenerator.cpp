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
#include <string>

PassGenerator::~PassGenerator()
{
}

void PassGenerator::init(const RLint renderWidth, const RLint renderHeight)
{
    // Fire up the render thread and push an init job to it to get going.
	auto runJob = [this](Job& job) {
		switch (job.type) {
			case JobType::kInit:
			{
				WindowSize size = std::any_cast<WindowSize>(job.params);
				if (!runInitJob(size.width, size.height)) {
					return true; // End the thread.
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
				bool clearOldScene = std::any_cast<bool>(job.params);
				runLoadSceneJob(clearOldScene);
				break;
			}
			case JobType::kDestroy:
			{
				runDestroyJob();
				return true; // End the thread.
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

		return false;
	};
	m_jobProcessor.init(std::move(runJob));

    WindowSize size(renderWidth, renderHeight);
    Job job(JobType::kInit, std::make_any<WindowSize>(size));
	m_jobProcessor.addTask(std::move(job));
}

void PassGenerator::destroy()
{
    Job job(JobType::kDestroy, std::make_any<void*>(nullptr));
	m_jobProcessor.addTask(std::move(job));

	m_jobProcessor.deinit();
}

void PassGenerator::resize(RLint newWidth, RLint newHeight)
{
    WindowSize size(newWidth, newHeight);
    Job job(JobType::kResize, std::make_any<WindowSize>(size));
	m_jobProcessor.addTask(std::move(job));
}

void PassGenerator::renderPass(const RenderOptions& newOptions, PassCompleteCallback callback)
{
    m_passCompleteCallback = callback;

    RenderOptions options = newOptions;
    Job job(JobType::kRenderPass, std::make_any<RenderOptions>(options));
	m_jobProcessor.addTask(std::move(job));
}

void PassGenerator::loadScene(LoadSceneCallback callback, bool clearOldScene)
{
    m_loadSceneCallback = callback;

    Job job(JobType::kLoadScene, std::make_any<bool>(clearOldScene));
	m_jobProcessor.addTask(std::move(job));
}

void PassGenerator::runOpenRLTask(OpenRLTask task)
{
	Job job(JobType::kGeneralTask, std::make_any<OpenRLTask>(task));
	m_jobProcessor.addTask(std::move(job));
}

bool PassGenerator::runInitJob(const RLint renderWidth, const RLint renderHeight)
{
    //OpenRLContextAttribute attributes[] = {kOpenRL_EnableRayPrefixShaders, 1, NULL};
    m_rlContext = OpenRLCreateContext(nullptr, nullptr, nullptr);
    RLFunc(OpenRLSetCurrentContext(m_rlContext));

    // Generate the lists of random sequences used for pathtracing.
    {
        generateRandomSequences(m_renderOptions.maxRenderPasses, m_renderOptions.sampleMode, m_renderOptions.bokehShape);
    }

    // Setup the buffers to render into.
    {
		m_fbo = openrl::Framebuffer::create();

        openrl::Texture::Descriptor desc;
        desc.dataType = RL_FLOAT;
        desc.format = RL_RGBA;
        desc.internalFormat = RL_RGBA;
        desc.width = renderWidth;
        desc.height = renderHeight;

        openrl::Texture::Sampler sampler;
        sampler.minFilter = RL_NEAREST;
        sampler.magFilter = RL_NEAREST;

        m_fboTexture = openrl::Texture::create(nullptr, desc, sampler, false);
        m_fbo->addAttachment(m_fboTexture, RL_COLOR_ATTACHMENT0);

        assert(m_fbo->valid());

        // Set this framebuffers as the primary source to render into.
        m_fbo->bind();

        RLint bufferSize = renderWidth * renderHeight * sizeof(float) * openrl::PixelPackBuffer::kNumChannels;
        m_resultPixels.create(bufferSize);
    }

	// Initialize scene lighting.
	{
		m_sceneLighting = std::shared_ptr<SceneLighting>(new SceneLighting);
		m_environmentLight = m_sceneLighting->addEnvironmentLight();

		// TMP HACK!!!
		//m_sceneLighting->addDirectionalLight();
	}

    {
        // Setup global data buffer for all shaders.
        GlobalData data;
        m_globalData = openrl::Buffer::create(RL_ARRAY_BUFFER, &data, sizeof(GlobalData), "Global data buffer");
    }

    // Load the perspective camera frame shader for generating primary rays.
    {
		std::stringstream defines;
		ShaderLightingDefines::appendLightingShaderDefines(defines);

        std::vector<std::string> shaderSource;
		shaderSource.push_back(defines.str());
		util::loadShaderSourceFile("perspective.rlsl", shaderSource);
		std::shared_ptr<openrl::Shader> frameShader = openrl::Shader::createFromMultipleStrings(shaderSource, openrl::Shader::ShaderType::kFrame, "Perspective Frame Shader");
        if (!frameShader) {
            return false;
        }

		m_frameProgram = openrl::Program::create();
        m_frameProgram->attach(frameShader);
        if (!m_frameProgram->link("Perspecive Frame Shader")) {
            return false;
        }

        // Heatray uses the null primitive as the frame primitive.
        RLFunc(rlBindPrimitive(RL_PRIMITIVE, RL_NULL_PRIMITIVE));
        m_frameProgram->bind();
        m_frameProgram->setUniformBlock(m_frameProgram->getUniformBlockIndex("RandomSequences"), m_randomSequences->buffer());
        m_frameProgram->setUniformBlock(m_frameProgram->getUniformBlockIndex("Globals"), m_globalData->buffer());
		m_sceneLighting->bindLightingBuffersToProgram(m_frameProgram);
    }

    return true;
}

void PassGenerator::runResizeJob(const RLint newRenderWidth, const RLint newRenderHeight)
{
    // Set the new viewport.
    rlViewport(0, 0, newRenderWidth, newRenderHeight);

    // Regenerate the render texture to be the proper size.
    if (m_fboTexture && m_fboTexture->valid()) {
        m_fboTexture->resize(newRenderWidth, newRenderHeight);

        if (m_resultPixels.mapped()) {
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
    if (m_resultPixels.mapped()) {
        m_resultPixels.unmapPixelData();
    }

    if ((newOptions.enableInteractiveMode != m_renderOptions.enableInteractiveMode) || 
        (newOptions.resetInternalState != m_renderOptions.resetInternalState)) {
        resetRenderingState(newOptions);
    }

    // Update global data for this frame.
    {
        m_globalData->bind();
        GlobalData* globalData = m_globalData->mapBuffer<GlobalData>();
        globalData->sampleIndex = m_currentSampleIndex;
        m_globalData->unmapBuffer();
        m_globalData->unbind();
    }
    
    glm::vec2 sensorDimensions(36.0f, 24.0f); // Dimensions of 35mm film.
    // https://en.wikipedia.org/wiki/Angle_of_view#Calculating_a_camera's_angle_of_view
    float fovY = 2.0f * std::atan2(sensorDimensions.y, 2.0f * m_renderOptions.camera.focalLength);

    m_frameProgram->bind();
    float fovTan = std::tanf(fovY * 0.5f);
    m_frameProgram->set1f(m_frameProgram->getUniformLocation("fovTan"), fovTan);
    m_frameProgram->set1f(m_frameProgram->getUniformLocation("aspectRatio"), m_renderOptions.camera.aspectRatio);
    m_frameProgram->set1f(m_frameProgram->getUniformLocation("focusDistance"), m_renderOptions.camera.focusDistance);
    m_frameProgram->set1f(m_frameProgram->getUniformLocation("apertureRadius"), m_renderOptions.camera.apertureRadius);
    m_frameProgram->setMatrix4fv(m_frameProgram->getUniformLocation("viewMatrix"), &(m_renderOptions.camera.viewMatrix[0][0]));
    m_frameProgram->set2iv(m_frameProgram->getUniformLocation("blockSize"), &m_renderOptions.kInteractiveBlockSize.x);
    m_frameProgram->set2iv(m_frameProgram->getUniformLocation("currentBlockPixel"), &m_currentBlockPixel.x);
    m_frameProgram->set1i(m_frameProgram->getUniformLocation("interactiveMode"), m_renderOptions.enableInteractiveMode ? 1 : 0);
    m_frameProgram->setTexture(m_frameProgram->getUniformLocation("apertureSamplesTexture"), m_apertureSamplesTexture);

    if (m_renderOptions.enableInteractiveMode) {
        m_currentBlockPixel.x += 1;
        if (m_currentBlockPixel.x == m_renderOptions.kInteractiveBlockSize.x) {
            m_currentBlockPixel.x = 0;
            m_currentBlockPixel.y += 1;
            if (m_currentBlockPixel.y == m_renderOptions.kInteractiveBlockSize.y) {
                m_currentBlockPixel = glm::vec2(0, 0);
                ++m_currentSampleIndex;
            }
        }
    } else
    {
        ++m_currentSampleIndex;
    }

    RLFunc(rlRenderFrame());
    m_resultPixels.setPixelData(*m_fboTexture);

    // Let the client know that a frame has been completed.
    float passTime = timer.stop();
    m_passCompleteCallback(m_resultPixels, passTime);
}

void PassGenerator::runLoadSceneJob(bool clearOldScene)
{
    assert(m_loadSceneCallback);

	if (clearOldScene) {
		for (auto& mesh : m_sceneData) {
			mesh.destroy();
		}
		m_sceneData.clear();
	}
	
    m_loadSceneCallback(m_sceneData, [this](const std::shared_ptr<openrl::Program> program)
        {
            RLint sequenceIndex = program->getUniformBlockIndex("RandomSequences");
            if (sequenceIndex != -1) {
                program->setUniformBlock(sequenceIndex, m_randomSequences->buffer());
            }

            RLint globalsIndex = program->getUniformBlockIndex("Globals");
            if (globalsIndex != -1) {
                program->setUniformBlock(globalsIndex, m_globalData->buffer());
            }

			m_sceneLighting->bindLightingBuffersToProgram(program);
        });
}

void PassGenerator::runDestroyJob()
{
    m_fbo.reset();
    m_fboTexture.reset();
    m_globalData.reset();
    m_randomSequences.reset();
    m_randomSequenceTexture.reset();
	m_apertureSamplesTexture.reset();
	m_environmentLight.reset();
	m_sceneData.clear();
	m_frameProgram.reset();
	m_sceneLighting.reset();

	m_resultPixels.unmapPixelData();
	m_resultPixels.destroy();

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
        m_globalData->bind();
        GlobalData* globalData = m_globalData->mapBuffer<GlobalData>();
        globalData->maxRayDepth = newOptions.maxRayDepth;
        m_globalData->unmapBuffer();
        m_globalData->unbind();
    }

	if (m_renderOptions.debugVisMode != newOptions.debugVisMode) {
		m_globalData->bind();
		GlobalData* globalData = m_globalData->mapBuffer<GlobalData>();
		// Reset all params.
		globalData->enableDebugVisualizer = 0;
		globalData->showGeometricNormals = 0;
		globalData->showUVs = 0;
		globalData->showTangents = 0;
		globalData->showBitangents = 0;
		globalData->showNormalmap = 0;
		globalData->showFinalNormals = 0;
		globalData->showBaseColor = 0;
		globalData->showRoughness = 0;
		globalData->showMetallic = 0;
		globalData->showEmissive = 0;
		globalData->showClearcoat = 0;
		globalData->showClearcoatRoughness = 0;
		globalData->showClearcoatNormalmap = 0;
		switch (newOptions.debugVisMode) {
			case RenderOptions::DebugVisualizationMode::kNone:
				globalData->enableDebugVisualizer = 0;
				break;
			case RenderOptions::DebugVisualizationMode::kGeometricNormals:
				globalData->enableDebugVisualizer = 1;
				globalData->showGeometricNormals = 1;
				break;
			case RenderOptions::DebugVisualizationMode::kUVs:
				globalData->enableDebugVisualizer = 1;
				globalData->showUVs = 1;
				break;
			case RenderOptions::DebugVisualizationMode::kTangents:
				globalData->enableDebugVisualizer = 1;
				globalData->showTangents = 1;
				break;
			case RenderOptions::DebugVisualizationMode::kBitangents:
				globalData->enableDebugVisualizer = 1;
				globalData->showBitangents = 1;
				break;
			case RenderOptions::DebugVisualizationMode::kNormalmap:
				globalData->enableDebugVisualizer = 1;
				globalData->showNormalmap = 1;
				break;
			case RenderOptions::DebugVisualizationMode::kFinalNormals:
				globalData->enableDebugVisualizer = 1;
				globalData->showFinalNormals = 1;
				break;
			case RenderOptions::DebugVisualizationMode::kBaseColor:
				globalData->enableDebugVisualizer = 1;
				globalData->showBaseColor = 1;
				break;
			case RenderOptions::DebugVisualizationMode::kRoughness:
				globalData->enableDebugVisualizer = 1;
				globalData->showRoughness = 1;
				break;
			case RenderOptions::DebugVisualizationMode::kMetallic:
				globalData->enableDebugVisualizer = 1;
				globalData->showMetallic = 1;
				break;
			case RenderOptions::DebugVisualizationMode::kEmissive:
				globalData->enableDebugVisualizer = 1;
				globalData->showEmissive = 1;
				break;
			case RenderOptions::DebugVisualizationMode::kClearcoat:
				globalData->enableDebugVisualizer = 1;
				globalData->showClearcoat = 1;
				break;
			case RenderOptions::DebugVisualizationMode::kClearcoatRoughness:
				globalData->enableDebugVisualizer = 1;
				globalData->showClearcoatRoughness = 1;
				break;
			case RenderOptions::DebugVisualizationMode::kClearcoatNormalmap:
				globalData->enableDebugVisualizer = 1;
				globalData->showClearcoatNormalmap = 1;
				break;
			default:
				break;
		}
		m_globalData->unmapBuffer();
		m_globalData->unbind();
	}

    // Finally get all of the new render options.
	m_renderOptions = newOptions;
    m_renderOptions.resetInternalState = false;
}

void PassGenerator::changeEnvironment(const RenderOptions::Environment &newEnv)
{
	if (!m_environmentLight) {
		m_environmentLight = m_sceneLighting->addEnvironmentLight();
	}

	m_environmentLight->rotate(newEnv.thetaRotation);
	m_environmentLight->setExposure(newEnv.exposureCompensation);

	if (newEnv.map == "white furnace test") {
		m_environmentLight->enableWhiteFurnaceTest();
	}
	else if (newEnv.map == "<none>") {
		m_sceneLighting->removeEnvironmentLight();
		m_environmentLight = nullptr;
	} else {
		m_environmentLight->changeImageSource(newEnv.map.c_str(), newEnv.builtInMap);
	}

	if (m_environmentLight) {
		m_sceneLighting->updateLight(m_environmentLight);
	}
}

void PassGenerator::generateRandomSequences(const RLint sampleCount, RenderOptions::SampleMode sampleMode, RenderOptions::BokehShape bokehShape)
{
    struct SequenceBlockData {
        RLtexture randomNumbers = RL_NULL_TEXTURE; ///< The actual sequence data stored in a 2D texture.
        RLfloat uvStep = 0.0f;  ///< The step to move forward to access the next sample (in UV space).
        RLfloat uvSequenceStep = 0.0f; ///< The step to move forward to access the next sequence (in UV space).
    };

    // If we already have previous sequence data, get rid of it.
    // Note that we don't delete the buffer however in order to keep all shader bindings valid.
    if (m_randomSequences && m_randomSequences->valid()) {
        m_randomSequenceTexture.reset();
    }
    else {
		// This is the first time this function is being called.
        SequenceBlockData dummyData;
		m_randomSequences = openrl::Buffer::create(RL_UNIFORM_BLOCK_BUFFER, &dummyData, sizeof(SequenceBlockData), "Random sequences uniform block");
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
    for (unsigned int iSequence = 0; iSequence < kNumRandomSequences; ++iSequence) {
        switch (sampleMode) {
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
                util::blueNoise(&values[iSequence * sampleCount], sampleCount, iSequence);
                break;
			case RenderOptions::SampleMode::kSobol:
				util::sobol(&values[iSequence * sampleCount], sampleCount, iSequence);
				break;
			default:
                assert("Unknown sample mode specified");
        }
    }

    // Load the data into a 2D texture and associate it with the overall sequence data.
    // The sequence data will be stored in a 2D texture that has the following layout:
    //      width  - number of samples in the sequence
    //      height -kNumRandomSequences -- therefore each row is a unique set of sequence data.
    m_randomSequenceTexture = openrl::Texture::create(&values[0], desc, sampler, false);

    m_randomSequences->bind();
    SequenceBlockData* sequences = m_randomSequences->mapBuffer<SequenceBlockData>(RL_WRITE_ONLY);
    sequences->randomNumbers = m_randomSequenceTexture->texture();
    sequences->uvStep = 1.0f / static_cast<RLfloat>(sampleCount);
    sequences->uvSequenceStep = 1.0f / static_cast<RLfloat>(kNumRandomSequences);
    m_randomSequences->unmapBuffer();
    m_randomSequences->unbind();

    // Now generate the data random sequence data for aperture sampling for depth of field.
    {
        std::vector<glm::vec3> values(kNumRandomSequences * sampleCount);
        for (unsigned int iSequence = 0; iSequence < kNumRandomSequences; ++iSequence) {
            switch (bokehShape) {
                case PassGenerator::RenderOptions::BokehShape::kCircular:
                    util::radialSobol(&values[iSequence * sampleCount], sampleCount, iSequence);
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

        m_apertureSamplesTexture = openrl::Texture::create(&values[0], desc, sampler, false);
    }
 }




