#define GLM_FORCE_SWIZZLE

#include "HeatrayRenderer.h"

#include "Lights/DirectionalLight.h"
#include "Lights/EnvironmentLight.h"
#include "Lights/PointLight.h"
#include "Lights/SpotLight.h"

#include "Materials/GlassMaterial.h"
#include "Materials/Material.h"
#include "Materials/MultiScatterUtil.h"
#include "Materials/PhysicallyBasedMaterial.h"
#include "Scene/AssimpMeshProvider.h"
#include "Scene/PlaneMeshProvider.h"
#include "Scene/SphereMeshProvider.h"
#include "Session/Session.h"

#include <RLWrapper/PixelPackBuffer.h>

#include <Utility/FileDialog.h>
#include <Utility/ImGuiLog.h>
#include <Utility/Random.h>
#include <Utility/TextureLoader.h>

#include <glm/glm/gtc/constants.hpp>
#include "imgui/imgui.h"
#include <FreeImage/FreeImage.h>

#include <assert.h>
#include <fstream>

bool HeatrayRenderer::init(const GLint windowWidth, const GLint windowHeight)
{
    m_windowParams.width = windowWidth;
    m_windowParams.height = windowHeight;

    // Setup OpenRL data first.
    m_renderWindowParams.width = windowWidth - UI_WINDOW_WIDTH;
    m_renderWindowParams.height = windowHeight;
    m_renderer.init(m_renderWindowParams.width, m_renderWindowParams.height);

    // Next setup OpenGL display data.
    {
        glewInit();

        // Create the pixel-pack buffer and gl texture to use for displaying the raytraced result.

        glGenBuffers(1, &m_displayPixelBuffer);
        glGenTextures(1, &m_displayTexture);

        // Setup the texture state.
        glBindTexture(GL_TEXTURE_2D, m_displayTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);

        resizeGLData();

        m_displayProgram.init();

        // Make sure that OpenGL doesn't automatically clamp our display texture. This is because the raytraced image
        // that will be stored is actually an accumulation of every ray that has gone through a given pixel.
        // Therefore each pixel value will quickly move beyond 1.0.  Only enabled on Windows as it doesn't seem to be
        // available on macOS.
#if defined(_WIN32) || defined(_WIN64)
        glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);
#endif
    }

    m_renderOptions.camera.aspectRatio = static_cast<float>(m_renderWindowParams.width) / static_cast<float>(m_renderWindowParams.height);
    m_renderOptions.camera.setApertureRadius();

    // Setup some defaults.
    m_renderOptions.environment.map = "studio.hdr";
    m_renderOptions.scene = "Multi-Material";
    m_renderOptions.camera.focusDistance = m_camera.orbitCamera.distance;
    m_renderOptions.camera.viewMatrix = m_camera.orbitCamera.createViewMatrix();

    // Load the default scene.
    changeScene(m_renderOptions.scene, true);
    resetRenderer();

    return true;
}

void HeatrayRenderer::destroy()
{
    // Run a job to destroy any possible RL objects we may have created.
    m_renderer.destroy();

    glDeleteBuffers(1, &m_displayPixelBuffer);
    glDeleteTextures(1, &m_displayTexture);
}

void HeatrayRenderer::resize(const GLint newWindowWidth, const GLint newWindowHeight)
{
    m_windowParams.width = newWindowWidth;
    m_windowParams.height = newWindowHeight;

    m_renderWindowParams.width = newWindowWidth - UI_WINDOW_WIDTH;
    m_renderWindowParams.height = newWindowHeight;

    m_renderer.resize(m_renderWindowParams.width, m_renderWindowParams.height);

    resizeGLData();

    m_renderOptions.camera.aspectRatio = static_cast<float>(m_renderWindowParams.width) / static_cast<float>(m_renderWindowParams.height);

    m_pathracedPixels.store(nullptr);
    m_shouldCopyPixels.store(false);
    m_justResized = true;
}

void HeatrayRenderer::changeScene(std::string const& sceneName, const bool moveCamera)
{
    m_groundPlane.exists = false;
    m_sceneTransform = SceneTransform();
    m_renderOptions.scene = sceneName;

    LOG_INFO("Loading scene: %s", sceneName.c_str());

    if (sceneName == "Editable PBR Material") {
        m_renderer.loadScene([this](std::shared_ptr<Scene> scene) {
            m_renderOptions.camera.focusDistance = m_camera.orbitCamera.distance; // Auto-focus to the center of the scene.

            SphereMeshProvider sphereMeshProvider(50, 50, 1.0f, "PBR Sphere");
            std::shared_ptr<PhysicallyBasedMaterial> material = std::make_shared<PhysicallyBasedMaterial>("PBR");
            PhysicallyBasedMaterial::Parameters &params = material->parameters();
            params.metallic = 0.0f;
            params.roughness = 1.0f;
            params.baseColor = glm::vec3(0.8f);
            params.specularF0 = 0.0f;
            params.clearCoat = 0.0f;
            params.clearCoatRoughness = 0.0f;
            params.forceEnableAllTextures = true;
            scene->addMesh(&sphereMeshProvider, { material }, glm::mat4(1.0f));
        });
    } else if (sceneName == "Editable Glass Material") {
        m_renderer.loadScene([this](std::shared_ptr<Scene> scene) {
            m_renderOptions.camera.focusDistance = m_camera.orbitCamera.distance; // Auto-focus to the center of the scene.

            SphereMeshProvider sphereMeshProvider(50, 50, 1.0f, "Glass Sphere");
            std::shared_ptr<GlassMaterial> material = std::make_shared<GlassMaterial>("Glass");
            GlassMaterial::Parameters& params = material->parameters();
            params.baseColor = glm::vec3(0.8f);
            params.ior = 1.33f;
            params.roughness = 0.0f;
            params.density = 0.8f;
            params.forceEnableAllTextures = true;
            scene->addMesh(&sphereMeshProvider, { material }, glm::mat4(1.0f));
        });
    } else if (sceneName == "Multi-Material") {
        m_renderer.loadScene([this](std::shared_ptr<Scene> scene) {
            m_renderOptions.camera.focusDistance = m_camera.orbitCamera.distance; // Auto-focus to the center of the scene.

            PlaneMeshProvider planeMeshProvider(15, 15, "Plane");
            glm::vec3 xAxis = glm::vec3(1.0f, 0.0f, 0.0f);
            glm::vec3 yAxis = glm::vec3(0.0f, 1.0f, 0.0f);
            glm::vec3 zAxis = glm::vec3(0.0f, 0.0f, 1.0f);

            // Bottom plane.
            {
                std::shared_ptr<PhysicallyBasedMaterial> material = std::make_shared<PhysicallyBasedMaterial>("Ground");
                PhysicallyBasedMaterial::Parameters &params = material->parameters();
                params.metallic = 0.0f;
                params.roughness = 1.0f;
                params.baseColor = glm::vec3(0.9f);
                params.specularF0 = 0.0f;
                glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.5f, 0.0f));
                scene->addMesh(&planeMeshProvider, { material }, translation);
            }
#if 0
            // Right plane.
            {
                std::shared_ptr<PhysicallyBasedMaterial> material = std::make_shared<PhysicallyBasedMaterial>();
                PhysicallyBasedMaterial::Parameters& params = material->parameters();
                params.metallic = 0.0f;
                params.roughness = 1.0f;
                params.baseColor = glm::vec3(0.9f);
                params.specularF0 = 0.0f;
                glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::half_pi<float>(), zAxis);
                glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(2.5f, 1.0f, 0.0f));
                sceneData.push_back(RLMesh(&planeMeshProvider, { material }, systemSetupCallback, translation * rotation));
            }

            // Back plane.
            {
                std::shared_ptr<PhysicallyBasedMaterial> material = std::make_shared<PhysicallyBasedMaterial>();
                PhysicallyBasedMaterial::Parameters& params = material->parameters();
                params.metallic = 0.0f;
                params.roughness = 1.0f;
                params.baseColor = glm::vec3(0.9f);
                params.specularF0 = 0.0f;
                glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::half_pi<float>(), xAxis);
                glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, -2.5f));
                sceneData.push_back(RLMesh(&planeMeshProvider, { material }, systemSetupCallback, translation * rotation));
            }

            // Left plane.
            /*{
                std::shared_ptr<PhysicallyBasedMaterial> material = std::make_shared<PhysicallyBasedMaterial>();
                PhysicallyBasedMaterial::Parameters &params = material->parameters();
                params.metallic = 0.0f;
                params.roughness = 0.8f;
                params.baseColor = glm::vec3(0.0f, 0.8f, 0.0f);
                params.specularF0 = 0.5f;
                glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::half_pi<float>() * 3.0f, zAxis);
                glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(-2.5f, 1.0f, 0.0f));
                sceneData.push_back(RLMesh(&planeMeshProvider, { material }, systemSetupCallback, translation * rotation));
            }*/
#endif

            float radius = 1.0f;
            SphereMeshProvider sphereMeshProvider(50, 50, radius, "Sphere");

            // Sphere 1.
            {
                std::shared_ptr<PhysicallyBasedMaterial> material = std::make_shared<PhysicallyBasedMaterial>("PBR");
                PhysicallyBasedMaterial::Parameters& params = material->parameters();;
                params.metallic = 1.0f;
                params.roughness = 0.1f;
                params.baseColor = glm::vec3(0.4f);
                params.specularF0 = 0.3f;
                glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(-0.9f, -0.5f, -0.8f));
                scene->addMesh(&sphereMeshProvider, { material }, translation);
            }

            // Sphere 2.
            {
                std::shared_ptr<GlassMaterial> material = std::make_shared<GlassMaterial>("Glass");
                GlassMaterial::Parameters& params = material->parameters();
                params.roughness = 0.1f;
                params.baseColor = glm::vec3(0.9f, 0.6f, 0.6f);
                params.ior = 1.57f;
                params.density = 0.5f;
                glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(1.2f, -0.5f, 0.8f));
                scene->addMesh(&sphereMeshProvider, { material }, translation);
            }
        });
    } else if (sceneName == "Sphere Array") {
        m_renderer.loadScene([this](std::shared_ptr<Scene> scene) {
            m_renderOptions.camera.focusDistance = m_camera.orbitCamera.distance; // Auto-focus to the center of the scene.

            float radius = 0.5f;
            SphereMeshProvider sphereMeshProvider(50, 50, radius, "Sphere");
            float roughness = 0.0f;
            float padding = radius * 0.2f;
            float startX = (-5.0f * (radius * 2.0f + padding)) + ((radius * 2.0f + padding) * 0.5f);
            
            // Non-metals.
            {
                for (int iSphere = 0; iSphere < 10; ++iSphere) {
                    std::shared_ptr<PhysicallyBasedMaterial> material = std::make_shared<PhysicallyBasedMaterial>(util::createStringWithFormat("Sphere dialectric roughness %f", roughness));
                    PhysicallyBasedMaterial::Parameters& params = material->parameters();
                    params.metallic = 0.0f;
                    params.roughness = roughness;
                    params.baseColor = glm::vec3(1.0f);
                    params.specularF0 = 0.0f;
                    glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(startX, 0.0f, 0.0f));
                    scene->addMesh(&sphereMeshProvider, { material }, translation);

                    roughness += 0.1f;
                    startX += radius * 2.0f + padding;
                }
            }

            // Metals.
            {
                roughness = 0.0f;
                startX = (-5.0f * (radius * 2.0f + padding)) + ((radius * 2.0f + padding) * 0.5f);
                for (int iSphere = 0; iSphere < 10; ++iSphere) {
                    std::shared_ptr<PhysicallyBasedMaterial> material = std::make_shared<PhysicallyBasedMaterial>(util::createStringWithFormat("Sphere conductor roughness %f", roughness));
                    PhysicallyBasedMaterial::Parameters& params = material->parameters();
                    params.metallic = 1.0f;
                    params.roughness = roughness;
                    params.baseColor = glm::vec3(1.0f);
                    params.specularF0 = 0.0f;
                    glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(startX, 1.5f, 0.0f));
                    scene->addMesh(&sphereMeshProvider, { material }, translation);

                    roughness += 0.1f;
                    startX += radius * 2.0f + padding;
                }
            }
        });
    } else {
        m_renderer.loadScene([this, sceneName, moveCamera](std::shared_ptr<Scene> scene) {
            scene->loadFromDisk(sceneName, (m_sceneUnits == SceneUnits::kCentimeters));

            // We'll automatically setup camera and AABB info if requested to do so.
            if (moveCamera) {
                m_sceneAABB = scene->aabb();
                updateCameraFromAABB();
            }
        });
    }
}

void HeatrayRenderer::updateCameraFromAABB()
{
    m_camera.orbitCamera.target = m_sceneAABB.center();
    m_camera.orbitCamera.distance = m_sceneAABB.radius() * 3.0f; // Add some extra scale.
    m_camera.orbitCamera.max_distance = m_sceneAABB.radius() * 10.0f;

    m_distanceScale = m_sceneAABB.radius();

    m_renderOptions.camera.focusDistance = m_camera.orbitCamera.distance; // Auto-focus to the center of the scene.
    resetRenderer(); // We must reset here in order to move the camera.
}

void HeatrayRenderer::changeEnvironment(std::string const& envMapPath)
{
    m_renderOptions.environment.map = envMapPath;
    m_renderOptions.environment.builtInMap = false;
    resetRenderer();
}

void HeatrayRenderer::render()
{
    // Copy the most recent frame (if necessary).
    bool copyPixels = m_shouldCopyPixels.load();
    if (!m_justResized && copyPixels) {
        // Copy the data into a PBO and upload it to a texture for rendering. If the renderer is being reset then no reason to actually copy anything.
        if (copyPixels && !m_renderOptions.resetInternalState) {
            // These may not be the same if a resize just happened - in that case we don't want to copy old data.
            if (m_renderWindowParams.width == m_pixelDimensions.x && m_renderWindowParams.height == m_pixelDimensions.y) {
                const float* pixels = m_pathracedPixels.load();
                m_shouldCopyPixels.store(false);
                glBindTexture(GL_TEXTURE_2D, m_displayTexture);
                glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_displayPixelBuffer);
                glBufferSubData(GL_PIXEL_UNPACK_BUFFER, 0, m_pixelDimensions.x * m_pixelDimensions.y * sizeof(float) * openrl::PixelPackBuffer::kNumChannels, pixels);

                glTexSubImage2D(GL_TEXTURE_2D,
                                0,
                                0,
                                0,
                                m_pixelDimensions.x,
                                m_pixelDimensions.y,
                                GL_RGBA,
                                GL_FLOAT,
                                nullptr);
                glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
                glBindTexture(GL_TEXTURE_2D, 0);
            }
        }
    }

    // Display the current raytraced result.
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_displayTexture);
        m_displayProgram.draw(0, m_post_processing_params, size_t(m_windowParams.width));
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // If requested to do a screenshot, perform it now.
    if (m_shouldSaveScreenshot) {
        saveScreenshot();
    }

    m_resetRequested |= renderUI() | m_cameraUpdated;
    m_cameraUpdated = false;

    // Kick the raytracer if necessary.
    if (!m_justResized && // Don't kick the renderer if we've just resized.
        ((!m_renderingFrame && (m_currentPass < m_totalPasses)) || // If we haven't yet rendered all passes and are not currently rendering a pass.
        (m_resetRequested && !m_renderingFrame))) { // If we've been asked to reset but are not in the middle of another frame.

        if (m_resetRequested) {
            resetRenderer();
        }

        bool skipRendering = false;
        if (m_renderOptions.debugPassRendering) {
            if (m_debugPassChanged) {
                m_debugPassChanged = false;
            } else {
                skipRendering = true;
            }
        }

        if (!skipRendering) {
            m_renderingFrame = true;
            m_shouldCopyPixels.store(false);
            // Tell the pathtracer to start generating a new frame.
            m_renderer.renderPass(m_renderOptions,
                [this](std::shared_ptr<openrl::PixelPackBuffer> results, float passTime, size_t passIndex)
                {
                    const float* pixelData = results->mapPixelData();
                    m_pathracedPixels.store(pixelData);
                    m_pixelDimensions = glm::ivec2(results->width(), results->height());
                    m_shouldCopyPixels.store(true);
                    m_renderingFrame = false;
                    m_currentPassTime = passTime;
                    m_totalRenderTime += passTime;
                    m_currentPass = passIndex;
                });

            m_renderOptions.resetInternalState = false;
            m_resetRequested = false;
        }
    }

    m_justResized = false;
}

void HeatrayRenderer::adjustCamera(const float phiDelta, const float thetaDelta, const float distanceDelta)
{
    if (!m_camera.locked) {
        float SCALE = 0.5f;

        m_camera.orbitCamera.phi += glm::radians(phiDelta) * SCALE;
        m_camera.orbitCamera.theta += glm::radians(thetaDelta) * SCALE;
        m_camera.orbitCamera.distance += distanceDelta * SCALE * m_distanceScale;

        // Ensure that the camera parameters are valid.
        {
            if (m_camera.orbitCamera.phi < 0.0f) {
                m_camera.orbitCamera.phi += glm::two_pi<float>();
            }
            else if (m_camera.orbitCamera.phi > glm::two_pi<float>()) {
                m_camera.orbitCamera.phi -= glm::two_pi<float>();
            }

            m_camera.orbitCamera.theta = glm::clamp(m_camera.orbitCamera.theta, -glm::half_pi<float>(), glm::half_pi<float>());
            m_camera.orbitCamera.distance = glm::clamp(m_camera.orbitCamera.distance, 0.0f, m_camera.orbitCamera.max_distance);

            m_renderOptions.camera.focusDistance = m_camera.orbitCamera.distance; // Auto-focus to the center of the scene.
        }

        m_cameraUpdated = true;
    }
}

void HeatrayRenderer::resizeGLData()
{
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_displayPixelBuffer);
    GLsizei bufferSize = m_renderWindowParams.width * m_renderWindowParams.height * sizeof(float) * openrl::PixelPackBuffer::kNumChannels;
    glBufferData(GL_PIXEL_UNPACK_BUFFER, bufferSize, nullptr, GL_STREAM_DRAW);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    glBindTexture(GL_TEXTURE_2D, m_displayTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, m_renderWindowParams.width, m_renderWindowParams.height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    glViewport(0, 0, m_windowParams.width, m_windowParams.height);
}

void HeatrayRenderer::generateSequenceVisualizationData(int sequenceIndex, int renderPasses, bool aperture)
{
    m_sequenceVisualizationData.clear();
    m_sequenceVisualizationData.resize(renderPasses);
    if (aperture) {
        switch (m_renderOptions.bokehShape) {
            case PassGenerator::RenderOptions::BokehShape::kCircular:
                util::radialSobol(&m_sequenceVisualizationData[0], renderPasses, sequenceIndex);
                break;
            case PassGenerator::RenderOptions::BokehShape::kPentagon:
                util::randomPolygonal(&m_sequenceVisualizationData[0], 5, renderPasses, sequenceIndex);
                break;
            case PassGenerator::RenderOptions::BokehShape::kHexagon:
                util::randomPolygonal(&m_sequenceVisualizationData[0], 6, renderPasses, sequenceIndex);
                break;
            case PassGenerator::RenderOptions::BokehShape::kOctagon:
                util::randomPolygonal(&m_sequenceVisualizationData[0], 8, renderPasses, sequenceIndex);
                break;
            default:
                assert(0);
        }
    } else {
        switch (m_renderOptions.sampleMode) {
            // Random and hammerlsey have no index to use.
            case PassGenerator::RenderOptions::SampleMode::kRandom:
                util::uniformRandomFloats<glm::vec2>(&m_sequenceVisualizationData[0], renderPasses, sequenceIndex, 0.0f, 1.0f);
                break;
            case PassGenerator::RenderOptions::SampleMode::kHalton:
                util::halton(&m_sequenceVisualizationData[0], renderPasses, sequenceIndex);
                break;
            case PassGenerator::RenderOptions::SampleMode::kHammersley:
                util::hammersley(&m_sequenceVisualizationData[0], renderPasses, sequenceIndex);
                break;
            case PassGenerator::RenderOptions::SampleMode::kBlueNoise:
                util::blueNoise(&m_sequenceVisualizationData[0], renderPasses, sequenceIndex);
                break;
            case PassGenerator::RenderOptions::SampleMode::kSobol:
                util::sobol(&m_sequenceVisualizationData[0], renderPasses, sequenceIndex);
                break;
            default:
                assert(0);
        }
    }
}

void HeatrayRenderer::writeSessionFile(const std::string& filename)
{
    Session session;

    // Set the various session variables that we want serialized to disk.
    
    // RenderOptions.
    {
        // General.
        session.setVariableValue(Session::SessionVariable::kInteractiveMode, m_renderOptions.enableInteractiveMode);
        session.setVariableValue(Session::SessionVariable::kInteractiveMode, m_renderOptions.enableOfflineMode);
        session.setVariableValue(Session::SessionVariable::kMaxRenderPasses, m_renderOptions.maxRenderPasses);
        session.setVariableValue(Session::SessionVariable::kMaxRayDepth, m_renderOptions.maxRayDepth);
        session.setVariableValue(Session::SessionVariable::kMaxChannelValue, m_renderOptions.maxChannelValue);
        session.setVariableValue(Session::SessionVariable::kScene, m_renderOptions.scene);
        session.setVariableValue(Session::SessionVariable::kSampleMode, static_cast<uint32_t>(m_renderOptions.sampleMode));
        session.setVariableValue(Session::SessionVariable::kBokehShape, static_cast<uint32_t>(m_renderOptions.bokehShape));

        // Environment.
        {
            session.setVariableValue(Session::SessionVariable::kEnvironmentMap, m_renderOptions.environment.map);
            session.setVariableValue(Session::SessionVariable::kEnvironmentBuiltIn, m_renderOptions.environment.builtInMap);
            session.setVariableValue(Session::SessionVariable::kEnvironmentExposureCompensation, m_renderOptions.environment.exposureCompensation);
            session.setVariableValue(Session::SessionVariable::kEnvironmentThetaRotation, m_renderOptions.environment.thetaRotation);
            session.setVariableValue(Session::SessionVariable::kEnvironmentMapSolidColorX, m_renderOptions.environment.solidColor.x);
            session.setVariableValue(Session::SessionVariable::kEnvironmentMapSolidColorY, m_renderOptions.environment.solidColor.y);
            session.setVariableValue(Session::SessionVariable::kEnvironmentMapSolidColorZ, m_renderOptions.environment.solidColor.z);
        }

        // Camera.
        {
            session.setVariableValue(Session::SessionVariable::kCameraAspectRatio, m_renderOptions.camera.aspectRatio);
            session.setVariableValue(Session::SessionVariable::kCameraFocusDistance, m_renderOptions.camera.focusDistance);
            session.setVariableValue(Session::SessionVariable::kCameraFocalLength, m_renderOptions.camera.focalLength);
            session.setVariableValue(Session::SessionVariable::kCameraApertureRadius, m_renderOptions.camera.apertureRadius);
            session.setVariableValue(Session::SessionVariable::kCameraFStop, m_renderOptions.camera.fstop);
        }
    }

    // Camera.
    {
        session.setVariableValue(Session::SessionVariable::kOrbitDistance, m_camera.orbitCamera.distance);
        session.setVariableValue(Session::SessionVariable::kOrbitPhi, m_camera.orbitCamera.phi);
        session.setVariableValue(Session::SessionVariable::kOrbitTheta, m_camera.orbitCamera.theta);
        session.setVariableValue(Session::SessionVariable::kOrbitTargetX, m_camera.orbitCamera.target.x);
        session.setVariableValue(Session::SessionVariable::kOrbitTargetY, m_camera.orbitCamera.target.y);
        session.setVariableValue(Session::SessionVariable::kOrbitTargetZ, m_camera.orbitCamera.target.z);
        session.setVariableValue(Session::SessionVariable::kOrbitMaxDistance, m_camera.orbitCamera.max_distance);
    }

    // Scene.
    {
        session.setVariableValue(Session::SessionVariable::kUnits, static_cast<uint32_t>(m_sceneUnits));
        session.setVariableValue(Session::SessionVariable::kAABB_MinX, m_sceneAABB.min.x);
        session.setVariableValue(Session::SessionVariable::kAABB_MinY, m_sceneAABB.min.y);
        session.setVariableValue(Session::SessionVariable::kAABB_MinZ, m_sceneAABB.min.z);
        session.setVariableValue(Session::SessionVariable::kAABB_MaxX, m_sceneAABB.max.x);
        session.setVariableValue(Session::SessionVariable::kAABB_MaxY, m_sceneAABB.max.y);
        session.setVariableValue(Session::SessionVariable::kAABB_MaxZ, m_sceneAABB.max.z);
        session.setVariableValue(Session::SessionVariable::kRotationYaw, m_sceneTransform.yaw);
        session.setVariableValue(Session::SessionVariable::kRotationPitch, m_sceneTransform.pitch);
        session.setVariableValue(Session::SessionVariable::kRotationRoll, m_sceneTransform.roll);
        session.setVariableValue(Session::SessionVariable::kScale, m_sceneTransform.scale);
    }

    // Post processing.
    {
        session.setVariableValue(Session::SessionVariable::kTonemapEnable, m_post_processing_params.tonemapping_enabled);
        session.setVariableValue(Session::SessionVariable::kExposure, m_post_processing_params.exposure);
        session.setVariableValue(Session::SessionVariable::kBrightness, m_post_processing_params.brightness);
        session.setVariableValue(Session::SessionVariable::kContrast, m_post_processing_params.contrast);
        session.setVariableValue(Session::SessionVariable::kHue, m_post_processing_params.hue);
        session.setVariableValue(Session::SessionVariable::kSaturation, m_post_processing_params.saturation);
        session.setVariableValue(Session::SessionVariable::kVibrance, m_post_processing_params.vibrance);
        session.setVariableValue(Session::SessionVariable::kRed, m_post_processing_params.red);
        session.setVariableValue(Session::SessionVariable::kGreen, m_post_processing_params.green);
        session.setVariableValue(Session::SessionVariable::kBlue, m_post_processing_params.blue);
    }

    session.writeSessionFile(filename);
}

void HeatrayRenderer::readSessionFile(const std::string& filename)
{
    Session session;
    if (session.parseSessionFile(filename)) {
        // RenderOptions.
        {
            // General.
            session.getVariableValue(Session::SessionVariable::kInteractiveMode, m_renderOptions.enableInteractiveMode);
            session.getVariableValue(Session::SessionVariable::kInteractiveMode, m_renderOptions.enableOfflineMode);
            session.getVariableValue(Session::SessionVariable::kMaxRenderPasses, m_renderOptions.maxRenderPasses);
            session.getVariableValue(Session::SessionVariable::kMaxRayDepth, m_renderOptions.maxRayDepth);
            session.getVariableValue(Session::SessionVariable::kMaxChannelValue, m_renderOptions.maxChannelValue);
            session.getVariableValue(Session::SessionVariable::kScene, m_renderOptions.scene);
            uint32_t tmp = 0;
            session.getVariableValue(Session::SessionVariable::kSampleMode, tmp);
            m_renderOptions.sampleMode = static_cast<PassGenerator::RenderOptions::SampleMode>(tmp);
            session.getVariableValue(Session::SessionVariable::kBokehShape, tmp);
            m_renderOptions.bokehShape = static_cast<PassGenerator::RenderOptions::BokehShape>(tmp);

            // Environment.
            {
                session.getVariableValue(Session::SessionVariable::kEnvironmentMap, m_renderOptions.environment.map);
                session.getVariableValue(Session::SessionVariable::kEnvironmentBuiltIn, m_renderOptions.environment.builtInMap);
                session.getVariableValue(Session::SessionVariable::kEnvironmentExposureCompensation, m_renderOptions.environment.exposureCompensation);
                session.getVariableValue(Session::SessionVariable::kEnvironmentThetaRotation, m_renderOptions.environment.thetaRotation);
                session.getVariableValue(Session::SessionVariable::kEnvironmentMapSolidColorX, m_renderOptions.environment.solidColor.x);
                session.getVariableValue(Session::SessionVariable::kEnvironmentMapSolidColorY, m_renderOptions.environment.solidColor.y);
                session.getVariableValue(Session::SessionVariable::kEnvironmentMapSolidColorZ, m_renderOptions.environment.solidColor.z);
            }

            // Camera.
            {
                session.getVariableValue(Session::SessionVariable::kCameraAspectRatio, m_renderOptions.camera.aspectRatio);
                session.getVariableValue(Session::SessionVariable::kCameraFocusDistance, m_renderOptions.camera.focusDistance);
                session.getVariableValue(Session::SessionVariable::kCameraFocalLength, m_renderOptions.camera.focalLength);
                session.getVariableValue(Session::SessionVariable::kCameraApertureRadius, m_renderOptions.camera.apertureRadius);
                session.getVariableValue(Session::SessionVariable::kCameraFStop, m_renderOptions.camera.fstop);
            }
        }

        // Camera.
        {
            session.getVariableValue(Session::SessionVariable::kOrbitDistance, m_camera.orbitCamera.distance);
            session.getVariableValue(Session::SessionVariable::kOrbitPhi, m_camera.orbitCamera.phi);
            session.getVariableValue(Session::SessionVariable::kOrbitTheta, m_camera.orbitCamera.theta);
            session.getVariableValue(Session::SessionVariable::kOrbitTargetX, m_camera.orbitCamera.target.x);
            session.getVariableValue(Session::SessionVariable::kOrbitTargetY, m_camera.orbitCamera.target.y);
            session.getVariableValue(Session::SessionVariable::kOrbitTargetZ, m_camera.orbitCamera.target.z);
            session.getVariableValue(Session::SessionVariable::kOrbitMaxDistance, m_camera.orbitCamera.max_distance);
        }

        // Scene.
        {
            uint32_t tmp = 0;
            session.getVariableValue(Session::SessionVariable::kUnits, tmp);
            m_sceneUnits = static_cast<SceneUnits>(tmp);
            session.getVariableValue(Session::SessionVariable::kAABB_MinX, m_sceneAABB.min.x);
            session.getVariableValue(Session::SessionVariable::kAABB_MinY, m_sceneAABB.min.y);
            session.getVariableValue(Session::SessionVariable::kAABB_MinZ, m_sceneAABB.min.z);
            session.getVariableValue(Session::SessionVariable::kAABB_MaxX, m_sceneAABB.max.x);
            session.getVariableValue(Session::SessionVariable::kAABB_MaxY, m_sceneAABB.max.y);
            session.getVariableValue(Session::SessionVariable::kAABB_MaxZ, m_sceneAABB.max.z);
            session.getVariableValue(Session::SessionVariable::kRotationYaw, m_sceneTransform.yaw);
            session.getVariableValue(Session::SessionVariable::kRotationPitch, m_sceneTransform.pitch);
            session.getVariableValue(Session::SessionVariable::kRotationRoll, m_sceneTransform.roll);
            session.getVariableValue(Session::SessionVariable::kScale, m_sceneTransform.scale);
        }

        // Post processing.
        {
            session.getVariableValue(Session::SessionVariable::kTonemapEnable, m_post_processing_params.tonemapping_enabled);
            session.getVariableValue(Session::SessionVariable::kExposure, m_post_processing_params.exposure);
            session.getVariableValue(Session::SessionVariable::kBrightness, m_post_processing_params.brightness);
            session.getVariableValue(Session::SessionVariable::kContrast, m_post_processing_params.contrast);
            session.getVariableValue(Session::SessionVariable::kHue, m_post_processing_params.hue);
            session.getVariableValue(Session::SessionVariable::kSaturation, m_post_processing_params.saturation);
            session.getVariableValue(Session::SessionVariable::kVibrance, m_post_processing_params.vibrance);
            session.getVariableValue(Session::SessionVariable::kRed, m_post_processing_params.red);
            session.getVariableValue(Session::SessionVariable::kGreen, m_post_processing_params.green);
            session.getVariableValue(Session::SessionVariable::kBlue, m_post_processing_params.blue);
        }

        // Now actually process the parameters. NOTE: we do not allow the camera to be reset because we want to use
        // the values present in the session file.
        changeScene(m_renderOptions.scene, false);
        
        // Ensure that the proper transform is applied to the scene AABB and the scene itself.
        {
            m_renderer.modifyScene([this](std::shared_ptr<Scene> scene) {
                glm::mat4 transform = m_sceneTransform.transform();
                scene->applyTransform(transform);
                
                if (m_sceneAABB.valid()) {
                    m_sceneAABB.transform = transform;
                    updateCameraFromAABB();
                }
            });
        }
        
        // NOTE: most everything is handled automatically during a reset.
        resetRenderer();
    }
}

bool HeatrayRenderer::renderMaterialEditor(std::shared_ptr<Material> material)
{
    enum class TextureType {
        kBaseColor,
        kMetallicRoughness,
        kClearCoat,
        kClearCoatRoughness
    } textureType;

    bool materialChanged = false;

    ImGui::Text("%s", material->name().c_str());
    if (material->type() == Material::Type::PBR) {
        std::shared_ptr<PhysicallyBasedMaterial> pbrMaterial = std::static_pointer_cast<PhysicallyBasedMaterial>(material);
        PhysicallyBasedMaterial::Parameters& parameters = pbrMaterial->parameters();

        if (ImGui::SliderFloat3("BaseColor", &parameters.baseColor.r, 0.0f, 1.0f)) {
            materialChanged = true;
        }
        if (ImGui::SliderFloat("Metallic", &parameters.metallic, 0.0f, 1.0f)) {
            materialChanged = true;
        }
        if (ImGui::SliderFloat("Roughness", &parameters.roughness, 0.0f, 1.0f)) {
            materialChanged = true;
        }
        if (ImGui::SliderFloat("SpecularF0", &parameters.specularF0, 0.0f, 1.0f)) {
            materialChanged = true;
        }
        if (ImGui::SliderFloat("ClearCoat", &parameters.clearCoat, 0.0f, 1.0f)) {
            materialChanged = true;
        }
        if (ImGui::SliderFloat("ClearCoat Roughness", &parameters.clearCoatRoughness, 0.0f, 1.0f)) {
            materialChanged = true;
        }

        ImGui::Separator();
        ImGui::Text("Textures");
        std::string texturePath;
        {
            bool textureSelected = false;

            ImGui::PushID("BaseColor");
            if (ImGui::Button("Load")) {
                textureSelected = true;
                textureType = TextureType::kBaseColor;
            }
            ImGui::PopID();
            ImGui::SameLine();
            ImGui::Text("BaseColor");

            ImGui::PushID("MetallicRoughness");
            if (ImGui::Button("Load")) {
                textureSelected = true;
                textureType = TextureType::kMetallicRoughness;
            }
            ImGui::PopID();
            ImGui::SameLine();
            ImGui::Text("MetallicRoughness");

            ImGui::PushID("ClearCoat");
            if (ImGui::Button("Load")) {
                textureSelected = true;
                textureType = TextureType::kClearCoat;
            }
            ImGui::PopID();
            ImGui::SameLine();
            ImGui::Text("ClearCoat");

            ImGui::PushID("ClearCoatRoughness");
            if (ImGui::Button("Load")) {
                textureSelected = true;
                textureType = TextureType::kClearCoatRoughness;
            }
            ImGui::PopID();
            ImGui::SameLine();
            ImGui::Text("ClearCoatRoughness");

            if (textureSelected) {
                std::vector<std::string> filenames = util::OpenFileDialog("*");

                if (filenames.size() > 0) {
                    texturePath = filenames[0];
                    materialChanged = true;
                }
            }
        }

        if (materialChanged) {
            m_renderer.runOpenRLTask([this, pbrMaterial, texturePath, textureType]() {
                if (texturePath.empty() == false) {
                    switch (textureType) {
                    case TextureType::kBaseColor:
                        pbrMaterial->parameters().baseColorTexture = util::loadTexture(texturePath.c_str(), true, true);
                        break;
                    case TextureType::kMetallicRoughness:
                        pbrMaterial->parameters().metallicRoughnessTexture = util::loadTexture(texturePath.c_str(), true, false);
                        break;
                    case TextureType::kClearCoat:
                        pbrMaterial->parameters().clearCoatTexture = util::loadTexture(texturePath.c_str(), true, false);
                        break;
                    case TextureType::kClearCoatRoughness:
                        pbrMaterial->parameters().clearCoatRoughnessTexture = util::loadTexture(texturePath.c_str(), true, false);
                        break;
                    default:
                        break;
                    }
                }

                pbrMaterial->modify();
            });
        }
    } else if (material->type() == Material::Type::Glass) {
        std::shared_ptr<GlassMaterial> glassMaterial = std::static_pointer_cast<GlassMaterial>(material);
        GlassMaterial::Parameters& parameters = glassMaterial->parameters();

        if (ImGui::SliderFloat3("BaseColor", &parameters.baseColor.r, 0.0f, 1.0f)) {
            materialChanged = true;
        }
        if (ImGui::SliderFloat("IOR", &parameters.ior, 1.0f, 4.0f)) {
            materialChanged = true;
        }
        if (ImGui::SliderFloat("Roughness", &parameters.roughness, 0.0f, 1.0f)) {
            materialChanged = true;
        }
        if (ImGui::SliderFloat("Density", &parameters.density, 0.0f, 20.0f)) {
            materialChanged = true;
        }

        ImGui::Separator();
        ImGui::Text("Textures");
        std::string texturePath;
        {
            bool textureSelected = false;

            ImGui::PushID("BaseColor");
            if (ImGui::Button("Load")) {
                textureSelected = true;
                textureType = TextureType::kBaseColor;
            }
            ImGui::PopID();
            ImGui::SameLine();
            ImGui::Text("BaseColor");

            ImGui::PushID("MetallicRoughness");
            if (ImGui::Button("Load")) {
                textureSelected = true;
                textureType = TextureType::kMetallicRoughness;
            }
            ImGui::PopID();
            ImGui::SameLine();
            ImGui::Text("MetallicRoughness");

            if (textureSelected) {
                std::vector<std::string> filenames = util::OpenFileDialog("*");

                if (filenames.size() > 0) {
                    texturePath = filenames[0];
                    materialChanged = true;
                }
            }
        }

        if (materialChanged) {
            m_renderer.runOpenRLTask([this, glassMaterial, texturePath, textureType]() {
                if (texturePath.empty() == false) {
                    switch (textureType) {
                    case TextureType::kBaseColor:
                        glassMaterial->parameters().baseColorTexture = util::loadTexture(texturePath.c_str(), true, true);
                        break;
                    case TextureType::kMetallicRoughness:
                        glassMaterial->parameters().metallicRoughnessTexture = util::loadTexture(texturePath.c_str(), true, false);
                        break;
                    default:
                        break;
                    }
                }

                glassMaterial->modify();
            });
        }
    }

    return materialChanged;
}

bool HeatrayRenderer::renderLightEditor(std::shared_ptr<Light> light)
{
    bool lightChanged = false;

    ImGui::Text("%s", light->name().c_str());
    if (light->type() == Light::Type::kDirectional) {
        std::shared_ptr<DirectionalLight> directLight = std::static_pointer_cast<DirectionalLight>(light);
        DirectionalLight::Params params = directLight->params();

        ImGui::PushID("LightColor");
        lightChanged |= ImGui::SliderFloat3("Color", &params.color.r, 0.0f, 1.0f);
        ImGui::PopID();
        lightChanged |= ImGui::SliderFloat("Illuminance (lm/(m^2))", &params.illuminance, 0.0f, 100000.0f);

        ImGui::PushID("LightTheta");
        lightChanged |= ImGui::SliderAngle("Theta", &params.orientation.theta, -90.0f, 90.0f);
        ImGui::PopID();

        ImGui::PushID("LightPhi");
        lightChanged |= ImGui::SliderAngle("Phi", &params.orientation.phi, 0.0f, 360.0f);
        ImGui::PopID();

        directLight->setParams(params);
    } else if (light->type() == Light::Type::kPoint) {
        std::shared_ptr<PointLight> pointLight = std::static_pointer_cast<PointLight>(light);
        PointLight::Params params = pointLight->params();

        lightChanged |= ImGui::InputFloat3("Position", &params.position.r);
        lightChanged |= ImGui::SliderFloat3("Color", &params.color.r, 0.0f, 1.0f);
        lightChanged |= ImGui::SliderFloat("Luminous Intensity (lm/sr)", &params.luminousIntensity, 0.0f, 100000.0f);

        pointLight->setParams(params);
    } else if (light->type() == Light::Type::kSpot) {
        std::shared_ptr<SpotLight> spotLight = std::static_pointer_cast<SpotLight>(light);
        SpotLight::Params params = spotLight->params();

        lightChanged |= ImGui::InputFloat3("Position", &params.position.r);
        lightChanged |= ImGui::SliderFloat3("Color", &params.color.r, 0.0f, 1.0f);
        lightChanged |= ImGui::SliderFloat("Luminous Intensity (lm/sr)", &params.luminousIntensity, 0.0f, 100000.0f);

        ImGui::PushID("LightTheta");
        lightChanged |= ImGui::SliderAngle("Theta", &params.orientation.theta, -90.0f, 90.0f);
        ImGui::PopID();

        ImGui::PushID("LightPhi");
        lightChanged |= ImGui::SliderAngle("Phi", &params.orientation.phi, 0.0f, 360.0f);
        ImGui::PopID();

        lightChanged |= ImGui::SliderAngle("Inner Angle", &params.innerAngle, 0.0f, glm::degrees(params.outerAngle));
        lightChanged |= ImGui::SliderAngle("Outer Angle", &params.outerAngle, params.innerAngle, 90.0f);

        spotLight->setParams(params);
    }

    if (lightChanged) {

        m_renderer.changeLighting([this, light](std::shared_ptr<Lighting> lighting) {
            if (light->type() == Light::Type::kDirectional) {
                std::shared_ptr<DirectionalLight> directLight = std::static_pointer_cast<DirectionalLight>(light);
                lighting->updateLight(directLight);
            } else if (light->type() == Light::Type::kPoint) {
                std::shared_ptr<PointLight> pointLight = std::static_pointer_cast<PointLight>(light);
                lighting->updateLight(pointLight);
            } else if (light->type() == Light::Type::kSpot) {
                std::shared_ptr<SpotLight> spotLight = std::static_pointer_cast<SpotLight>(light);
                lighting->updateLight(spotLight);
            }
        });
    }

    return lightChanged;
}

bool HeatrayRenderer::renderUI()
{
    bool shouldResetRenderer = m_justResized;

    bool should_close_window = false;
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(UI_WINDOW_WIDTH, static_cast<float>(m_windowParams.height)));
    ImGui::Begin("Main Menu", &should_close_window, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);

    ImGui::Text("Render stats:");
    {
        ImGui::Text("Passes completed: %u\n", uint32_t(float(m_currentPass) / float(m_totalPasses) * float(m_renderOptions.maxRenderPasses)));
        ImGui::Text("Pass time(s): %f\n", m_currentPassTime);
        ImGui::Text("Total render time(s): %f\n", m_totalRenderTime);
    }
    if (ImGui::CollapsingHeader("Session")) {
        ImGui::PushID("Session_Save");
        if (ImGui::Button("Save")) {
            std::vector<std::string> filepaths = util::SaveFileDialog("xml");
            if (!filepaths.empty()) {
                writeSessionFile(filepaths[0]);
            }
        }
        ImGui::PopID();
        ImGui::SameLine();
        if (ImGui::Button("Load")) {
            std::vector<std::string> filepaths = util::OpenFileDialog("xml");
            if (!filepaths.empty()) {
                readSessionFile(filepaths[0]);
            }
        }
    }
    if (ImGui::CollapsingHeader("Render options")) {
        {
            static const char* options[] = { "1", "2", "4", "8", "16", "32", "64", "128", "256", "512", "1024", "2048", "4096", "8192" };
            constexpr unsigned int realOptions[] = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192 };

            static unsigned int currentSelection = 5;
            if (ImGui::BeginCombo("Max number of passes", options[currentSelection])) {
                for (int iOption = 0; iOption < sizeof(options) / sizeof(options[0]); ++iOption) {
                    bool isSelected = currentSelection == iOption;
                    if (ImGui::Selectable(options[iOption], false)) {
                        currentSelection = iOption;
                        m_renderOptions.maxRenderPasses = realOptions[currentSelection];
                        shouldResetRenderer = true;
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            if (ImGui::SliderInt("Max ray depth", (int*)(&m_renderOptions.maxRayDepth), 1, 30)) {
                shouldResetRenderer = true;
            }
            if (ImGui::Checkbox("Enable interactive mode", &m_renderOptions.enableInteractiveMode)) {
                shouldResetRenderer = true;
                m_renderOptions.enableOfflineMode = false; // These are radial options.
            } else if (ImGui::Checkbox("Enable offline mode", &m_renderOptions.enableOfflineMode)) {
                shouldResetRenderer = true;
                m_renderOptions.enableInteractiveMode = false; // These are radial options.
            }
            if (ImGui::SliderFloat("Max channel value", &(m_renderOptions.maxChannelValue), 0.0f, 10.0f)) {
                shouldResetRenderer = true;
            }
        }
        {
            static const char* options[] = { "Pseudo-random", "Halton", "Hammersley", "Blue Noise", "Sobol", };
            constexpr PassGenerator::RenderOptions::SampleMode realOptions[] = { 
                PassGenerator::RenderOptions::SampleMode::kRandom,
                PassGenerator::RenderOptions::SampleMode::kHalton,
                PassGenerator::RenderOptions::SampleMode::kHammersley,
                PassGenerator::RenderOptions::SampleMode::kBlueNoise,
                PassGenerator::RenderOptions::SampleMode::kSobol
            };
            static unsigned int currentSelection = static_cast<unsigned int>(m_renderOptions.sampleMode);
            if (ImGui::BeginCombo("Sampling technique", options[currentSelection])) {
                for (int iOption = 0; iOption < sizeof(options) / sizeof(options[0]); ++iOption) {
                    bool isSelected = currentSelection == iOption;
                    if (ImGui::Selectable(options[iOption], false)) {
                        currentSelection = iOption;
                        m_renderOptions.sampleMode = realOptions[currentSelection];
                        shouldResetRenderer = true;
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            if (ImGui::Checkbox("Visualize sequence", &m_visualizeSequenceData)) {
                if (m_visualizeSequenceData) {
                    generateSequenceVisualizationData(0, m_renderOptions.maxRenderPasses, false);
                }
            }
        }
    }
    if (ImGui::CollapsingHeader("Environment options")) {
        static const char* options[] = { "<none>", EnvironmentLight::SOLID_COLOR, "studio.hdr", "noon_grass.exr", "kloppenheim.exr", "peppermint_powerplant.exr", "urban_alley.exr", "chinese_garden.exr", "glacier.exr", "uffizi.exr", "Load Custom..." };
        static constexpr size_t NUM_OPTIONS = sizeof(options) / sizeof(options[0]);
        static constexpr size_t CUSTOM_OPTION_INDEX = NUM_OPTIONS - 1;
        static constexpr size_t SOLID_COLOR_OPTION_INDEX = 1;

        static size_t currentSelection = 2;
        if (ImGui::BeginCombo("Environment map", options[currentSelection])) {
            for (size_t iOption = 0; iOption < NUM_OPTIONS; ++iOption) {
                bool isSelected = (currentSelection == iOption);
                if (ImGui::Selectable(options[iOption], false)) {
                    currentSelection = iOption;
                    if (currentSelection != CUSTOM_OPTION_INDEX) {
                        m_renderOptions.environment.map = std::string(options[iOption]);
                        m_renderOptions.environment.builtInMap = true;
                    } else {
                        std::vector<std::string> filenames = util::OpenFileDialog("*");

                        if (filenames.size() > 0) {
                            m_renderOptions.environment.map = filenames[0];
                            m_renderOptions.environment.builtInMap = false;
                        }
                    }
                    shouldResetRenderer = true;
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        if (currentSelection == SOLID_COLOR_OPTION_INDEX) {
            ImGui::PushID("SolidColor");
            if (ImGui::SliderFloat3("Color", &m_renderOptions.environment.solidColor.r, 0.0f, 1.0f)) {
                shouldResetRenderer = true;
            }
            ImGui::PopID();
        }

        if (ImGui::SliderAngle("Environment Rotation", &m_renderOptions.environment.thetaRotation, 0.0f, 360.0f)) {
            shouldResetRenderer = true;
        }

        if (ImGui::SliderFloat("Environment Exposure", &m_renderOptions.environment.exposureCompensation, -10.0f, 10.0f)) {
            shouldResetRenderer = true;
        }
    }
    if (ImGui::CollapsingHeader("Scene options")) {
        ImGui::Text("Scene Units:");
        if (ImGui::RadioButton("Meters", m_sceneUnits == SceneUnits::kMeters)) {
            m_sceneUnits = SceneUnits::kMeters;
        }
        if (ImGui::RadioButton("Centimeters", m_sceneUnits == SceneUnits::kCentimeters)) {
            m_sceneUnits = SceneUnits::kCentimeters;
        }

        static const char* options[] = { "Sphere Array", "Multi-Material", "Editable PBR Material", "Editable Glass Material", "Load Custom..."};
        static constexpr size_t NUM_OPTIONS = sizeof(options) / sizeof(options[0]);
        static constexpr size_t CUSTOM_OPTION_INDEX = NUM_OPTIONS - 1;

        static unsigned int currentSelection = 1;
        if (ImGui::BeginCombo("Built-In Scenes", options[currentSelection])) {
            for (int iOption = 0; iOption < sizeof(options) / sizeof(options[0]); ++iOption) {
                bool isSelected = currentSelection == iOption;
                if (ImGui::Selectable(options[iOption], false)) {
                    currentSelection = iOption;
                    bool newScene = false;
                    if (currentSelection == CUSTOM_OPTION_INDEX) {
                        std::vector<std::string> filenames = util::OpenFileDialog("*");

                        if (filenames.size() > 0) {
                            m_renderOptions.scene = filenames[0];
                            newScene = true;
                        }
                    } else {
                        newScene = true;
                        m_renderOptions.scene = options[iOption];
                    }
                    if (newScene) {
                        changeScene(m_renderOptions.scene, true);
                        shouldResetRenderer = true;
                    }
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        // Scene rotations.
        {
            ImGui::Separator();
            ImGui::Text("Scene Transform");
            bool transformChanged = false;
            transformChanged |= ImGui::SliderAngle("Yaw", &m_sceneTransform.yaw, 0.0f, 360.0f);
            transformChanged |= ImGui::SliderAngle("Pitch", &m_sceneTransform.pitch, 0.0f, 360.0f);
            transformChanged |= ImGui::SliderAngle("Roll", &m_sceneTransform.roll, 0.0f, 360.0f);
            transformChanged |= ImGui::InputFloat("Scale", &m_sceneTransform.scale);
            ImGui::Separator();

            if (transformChanged) {
                glm::mat4 transform = m_sceneTransform.transform();
                m_renderer.modifyScene([this, transform](std::shared_ptr<Scene> scene) {
                    scene->applyTransform(transform);
                    
                    if (m_sceneAABB.valid()) {
                        // Also transform the scene AABB and be sure to update it.
                        m_sceneAABB.transform = transform;
                        updateCameraFromAABB();
                    }
                });

                shouldResetRenderer = true;
            }
        }

        static std::string NONE = "<none>";
        static std::string currentlySelectedMaterialName = NONE;
        static std::weak_ptr<Material> currentlySelectedMaterial;

        if (ImGui::Button(m_groundPlane.exists ? "Remove Ground Plane" : "Add Ground Plane")) {
            bool deleteGroundPlane = m_groundPlane.exists;
            m_groundPlane.exists = !m_groundPlane.exists;
            if (!m_groundPlane.exists) {
                currentlySelectedMaterial.reset();
                currentlySelectedMaterialName = NONE;
            }
            m_renderer.modifyScene([this, deleteGroundPlane](std::shared_ptr<Scene> scene) {
                if (deleteGroundPlane) {
                    scene->removeMesh(m_groundPlane.meshIndex);
                } else {
                    size_t planeSize = std::max(size_t(1), size_t(m_sceneAABB.radius())) * 5;
                    PlaneMeshProvider planeMeshProvider(planeSize, planeSize, "Ground Plane");

                    std::shared_ptr<PhysicallyBasedMaterial> material = std::make_shared<PhysicallyBasedMaterial>("Ground Plane");
                    PhysicallyBasedMaterial::Parameters& params = material->parameters();
                    params.metallic = 0.0f;
                    params.roughness = 0.9f;
                    params.baseColor = glm::vec3(0.9f);
                    params.specularF0 = 0.2f;
                    params.forceEnableAllTextures = true;
                    glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, m_sceneAABB.bottom(), 0.0f));

                    m_groundPlane.meshIndex = scene->addMesh(&planeMeshProvider, { material }, translation);
                }

                resetRenderer();
            });
        }

        static std::string currentlySelectedLightName = NONE;
        static std::weak_ptr<Light> currentlySelectedLight;

        // Extra lighting.
        {
            ImGui::Separator();
            static Light::Type selectedType = Light::Type::kDirectional;
            ImGui::Text("Additional Lighting");
            if (ImGui::RadioButton("Directional", selectedType == Light::Type::kDirectional)) {
                selectedType = Light::Type::kDirectional;
            }
            if (ImGui::RadioButton("Point", selectedType == Light::Type::kPoint)) {
                selectedType = Light::Type::kPoint;
            }
            if (ImGui::RadioButton("Spot", selectedType == Light::Type::kSpot)) {
                selectedType = Light::Type::kSpot;
            }

            static char lightName[256] = { "Scene Light" };
            ImGui::InputText("Name", lightName, 256);
            if (ImGui::Button("Add Light")) {
                currentlySelectedLight.reset();
                currentlySelectedLightName = NONE;
                m_renderer.changeLighting([this](std::shared_ptr<Lighting> lighting) {
                    switch (selectedType) {
                        case Light::Type::kDirectional:
                            lighting->addDirectionalLight(lightName);
                            break;
                        case Light::Type::kPoint:
                            lighting->addPointLight(lightName);
                            break;
                        case Light::Type::kSpot:
                            lighting->addSpotLight(lightName);
                            break;
                        default:
                            break;
                    }
                    
                    resetRenderer();
                });
            }

            if (ImGui::Button("Clear Lighting")) {
                currentlySelectedLight.reset();
                currentlySelectedLightName = NONE;
                m_renderer.changeLighting([this](std::shared_ptr<Lighting> lighting) {
                    lighting->clearAllButEnvironment();
                    resetRenderer();
                });
            }
        }

        ImGui::Separator();
        ImGui::Text("Editors");
        {
            if (ImGui::BeginCombo("Materials", currentlySelectedMaterialName.c_str())) {
                if (ImGui::Selectable(NONE.c_str(), false)) {
                    currentlySelectedMaterial.reset();
                } else {
                    const std::vector<Mesh>& sceneData = m_renderer.scene()->meshes();
                    for (auto& mesh : sceneData) {
                        for (auto& material : mesh.materials()) {
                            if (ImGui::Selectable(material->name().c_str(), false)) {
                                currentlySelectedMaterialName = material->name();
                                currentlySelectedMaterial = material;
                                break;
                            }
                        }
                    }
                }
                ImGui::EndCombo();
            }

            std::shared_ptr<Material> validMaterial = currentlySelectedMaterial.lock();
            if (validMaterial) {
                ImGui::Separator();
                shouldResetRenderer |= renderMaterialEditor(validMaterial);		
            }
        }

        {
            if (ImGui::BeginCombo("Lights", currentlySelectedLightName.c_str())) {
                if (ImGui::Selectable(NONE.c_str(), false)) {
                    currentlySelectedLight.reset();
                    currentlySelectedLightName = NONE;
                } else {
                    std::shared_ptr<Lighting> lighting = m_renderer.scene()->lighting();
                    const std::shared_ptr<DirectionalLight> *directional = lighting->directionalLights();
                    const std::shared_ptr<PointLight> *point = lighting->pointLights();
                    const std::shared_ptr<SpotLight> *spot = lighting->spotLights();

                    for (size_t iLight = 0; iLight < ShaderLightingDefines::MAX_NUM_DIRECTIONAL_LIGHTS; ++iLight) {
                        if (directional[iLight]) {
                            if (ImGui::Selectable(directional[iLight]->name().c_str(), false)) {
                                currentlySelectedLightName = directional[iLight]->name();
                                currentlySelectedLight = directional[iLight];
                                break;
                            }
                        } else {
                            break;
                        }
                    }

                    for (size_t iLight = 0; iLight < ShaderLightingDefines::MAX_NUM_POINT_LIGHTS; ++iLight) {
                        if (point[iLight]) {
                            if (ImGui::Selectable(point[iLight]->name().c_str(), false)) {
                                currentlySelectedLightName = point[iLight]->name();
                                currentlySelectedLight = point[iLight];
                                break;
                            }
                        } else {
                            break;
                        }
                    }

                    for (size_t iLight = 0; iLight < ShaderLightingDefines::MAX_NUM_SPOT_LIGHTS; ++iLight) {
                        if (spot[iLight]) {
                            if (ImGui::Selectable(spot[iLight]->name().c_str(), false)) {
                                currentlySelectedLightName = spot[iLight]->name();
                                currentlySelectedLight = spot[iLight];
                                break;
                            }
                        } else {
                            break;
                        }
                    }
                }
                ImGui::EndCombo();
            }

            std::shared_ptr<Light> validLight = currentlySelectedLight.lock();
            if (validLight) {
                ImGui::Separator();
                shouldResetRenderer |= renderLightEditor(validLight);
            }
        }

        // Debug visualizations.
        ImGui::Separator();
        ImGui::Text("Debug");
        {
            static const char* options[] = { "None", "Geometric Normals", "UVs", "Tangnents", "Bitangents", "Normalmap", "Final Normals",
                                             "Base color", "Roughness", "Metallic", "Emissive", "Clearcoat", "Clearcoat roughness", "Clearcoat normalmap", "Shader",
                                             "NANs", "Inf"};
            constexpr PassGenerator::RenderOptions::DebugVisualizationMode realOptions[] = {
                PassGenerator::RenderOptions::DebugVisualizationMode::kNone,
                PassGenerator::RenderOptions::DebugVisualizationMode::kGeometricNormals,
                PassGenerator::RenderOptions::DebugVisualizationMode::kUVs,
                PassGenerator::RenderOptions::DebugVisualizationMode::kTangents,
                PassGenerator::RenderOptions::DebugVisualizationMode::kBitangents,
                PassGenerator::RenderOptions::DebugVisualizationMode::kNormalmap,
                PassGenerator::RenderOptions::DebugVisualizationMode::kFinalNormals,
                PassGenerator::RenderOptions::DebugVisualizationMode::kBaseColor,
                PassGenerator::RenderOptions::DebugVisualizationMode::kRoughness,
                PassGenerator::RenderOptions::DebugVisualizationMode::kMetallic,
                PassGenerator::RenderOptions::DebugVisualizationMode::kEmissive,
                PassGenerator::RenderOptions::DebugVisualizationMode::kClearcoat,
                PassGenerator::RenderOptions::DebugVisualizationMode::kClearcoatRoughness,
                PassGenerator::RenderOptions::DebugVisualizationMode::kClearcoatNormalmap,
                PassGenerator::RenderOptions::DebugVisualizationMode::kShader,
                PassGenerator::RenderOptions::DebugVisualizationMode::kNANs,
                PassGenerator::RenderOptions::DebugVisualizationMode::kInf
            };

            static unsigned int currentSelection = 0;
            if (ImGui::BeginCombo("Debug Visualization", options[currentSelection])) {
                for (int iOption = 0; iOption < sizeof(options) / sizeof(options[0]); ++iOption) {
                    bool isSelected = currentSelection == iOption;
                    if (ImGui::Selectable(options[iOption], false)) {
                        currentSelection = iOption;
                        m_renderOptions.debugVisMode = realOptions[iOption];
                        shouldResetRenderer = true;
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        }
    }
    if (ImGui::CollapsingHeader("Camera options")) {
        ImGui::Checkbox("Lock camera", &(m_camera.locked));
        if (!m_camera.locked) {
            ImGui::Text("Orbital Camera");
            ImGui::PushID("CameraPhi");
            bool changed = ImGui::SliderAngle("Phi", &(m_camera.orbitCamera.phi), 0.0f, 360.0f);
            ImGui::PopID();
            ImGui::PushID("CameraTheta");
            changed |= ImGui::SliderAngle("Theta", &(m_camera.orbitCamera.theta), -90.0f, 90.0f);
            ImGui::PopID();
            changed |= ImGui::SliderFloat("Distance", &(m_camera.orbitCamera.distance), 0.0f, m_camera.orbitCamera.max_distance);
            changed |= ImGui::InputFloat3("Target", &m_camera.orbitCamera.target.r, "%f", ImGuiInputTextFlags_CharsDecimal);
            if (changed) {
                shouldResetRenderer = true;
            }

            ImGui::Text("Lens Properties");
            if (ImGui::SliderFloat("Focus distance(m)", &m_renderOptions.camera.focusDistance, 0.0f, m_camera.orbitCamera.max_distance)) {
                shouldResetRenderer = true;
            }
            if (ImGui::SliderFloat("Focal length(mm)", &m_renderOptions.camera.focalLength, 10.0f, 200.0f)) {
                m_renderOptions.camera.setApertureRadius();
                shouldResetRenderer = true;
            }
            {
                static unsigned int currentSelection = 1;
                static const char* options[PassGenerator::RenderOptions::Camera::NUM_FSTOPS] = {
                    "<disabled>", "f32", "f22", "f16", "f11", "f8", "f5.6", "f4", "f2.8", "f2", "f1.4", "f1"
                };
                if (ImGui::BeginCombo("f-Stop", options[currentSelection])) {
                    for (int iOption = 0; iOption < sizeof(options) / sizeof(options[0]); ++iOption) {
                        bool isSelected = currentSelection == iOption;
                        if (ImGui::Selectable(options[iOption], false)) {
                            currentSelection = iOption;
                            m_renderOptions.camera.fstop = PassGenerator::RenderOptions::Camera::fstopOptions[currentSelection];
                            m_renderOptions.camera.setApertureRadius();
                            shouldResetRenderer = true;
                        }
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
            }
            {
                static const char* options[] = { "Circular", "Pentagon", "Hexagon", "Octagon" };
                constexpr PassGenerator::RenderOptions::BokehShape realOptions[] = {
                    PassGenerator::RenderOptions::BokehShape::kCircular,
                    PassGenerator::RenderOptions::BokehShape::kPentagon,
                    PassGenerator::RenderOptions::BokehShape::kHexagon,
                    PassGenerator::RenderOptions::BokehShape::kOctagon
                };

                static unsigned int currentSelection = 0;
                if (ImGui::BeginCombo("Bokeh shape", options[currentSelection])) {
                    for (int iOption = 0; iOption < sizeof(options) / sizeof(options[0]); ++iOption) {
                        bool isSelected = currentSelection == iOption;
                        if (ImGui::Selectable(options[iOption], false)) {
                            currentSelection = iOption;
                            m_renderOptions.bokehShape = realOptions[iOption];
                            shouldResetRenderer = true;
                        }
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
            }
        }
    }
    if (ImGui::CollapsingHeader("Post processing")) {
        ImGui::Checkbox("ACES tonemapping enabled", &m_post_processing_params.tonemapping_enabled);
        ImGui::SliderFloat("Exposure compensation", &m_post_processing_params.exposure, -10.0f, 10.0f);
        ImGui::SliderFloat("Brightness", &m_post_processing_params.brightness, -1.0f, 1.0f);
        ImGui::SliderFloat("Contrast", &m_post_processing_params.contrast, -1.0f, 2.0f);
        ImGui::SliderFloat("Saturation", &m_post_processing_params.saturation, 0.0f, 3.0f);
        ImGui::SliderFloat("Vibrance", &m_post_processing_params.vibrance, 0.0f, 1.0f);
        ImGui::SliderFloat("Red", &m_post_processing_params.red, 0.0f, 1.5f);
        ImGui::SliderFloat("Green", &m_post_processing_params.green, 0.0f, 1.5f);
        ImGui::SliderFloat("Blue", &m_post_processing_params.blue, 0.0f, 1.5f);
        if (ImGui::Button("Reset parameters")) {
            m_post_processing_params = PostProcessingParams();
        }
    }
    if (ImGui::CollapsingHeader("Screenshot")) {
        ImGui::Checkbox("HDR", &m_hdrScreenshot);
        ImGui::PushID("Screenshot_Save");
        if (ImGui::Button("Save")) {
            std::vector<std::string> names = util::SaveFileDialog(m_hdrScreenshot ? "tiff" : "png");
            if (!names.empty()) {
                m_screenshotPath = names[0];
                m_shouldSaveScreenshot = true; // Do this on the next frame before drawing UI.
            }
        }
        ImGui::PopID();
    }
    if (ImGui::CollapsingHeader("Developer")) {
        if (ImGui::Button("Generate MultiScatter LUT")) {
            generateMultiScatterTexture();
        }

        // UI to handle debug pass rendering.
        {
            if (ImGui::Checkbox("Pass Debugging", &m_renderOptions.debugPassRendering)) {
                shouldResetRenderer = true;
            }

            if (m_renderOptions.debugPassRendering) {
                if (ImGui::SliderInt("Pass", &m_renderOptions.debugPassIndex, 0, (int)m_totalPasses)) {
                    shouldResetRenderer = true;
                    m_debugPassChanged = true;
                }
            } else {
                m_renderOptions.debugPassIndex = 0;
            }
        }
    }

    // Console log.
    {
        ImGui::Separator();
        ImGui::Text("Output Console");
        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            std::static_pointer_cast<util::ImGuiLog>(util::Log::instance())->clear();
        }
        ImGui::SameLine();
        if (ImGui::Button("Write To Disk")) {
            std::vector<std::string> names = util::SaveFileDialog("txt");
                if (!names.empty()) {
                    std::ofstream fout;
                    fout.open(names[0]);
                    if (fout) {
                        fout << "INFO:" << std::endl;
                        fout << std::static_pointer_cast<util::ImGuiLog>(util::Log::instance())->textBuffer(util::Log::Type::kInfo).c_str() << std::endl;
                        fout << "WARNINGS:" << std::endl;
                        fout << std::static_pointer_cast<util::ImGuiLog>(util::Log::instance())->textBuffer(util::Log::Type::kWarning).c_str() << std::endl;
                        fout << "ERRORS:" << std::endl;
                        fout << std::static_pointer_cast<util::ImGuiLog>(util::Log::instance())->textBuffer(util::Log::Type::kError).c_str() << std::endl;
                        fout.close();
                    }
                }
        }
        ImGui::Separator();
        ImGui::BeginChild("Info Console Log", ImVec2(0.0f, -1.0f), false, ImGuiWindowFlags_AlwaysHorizontalScrollbar);
        {
            ImGui::Text("Info");
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            ImGui::TextUnformatted(std::static_pointer_cast<util::ImGuiLog>(util::Log::instance())->textBuffer(util::Log::Type::kInfo).begin());
            ImGui::PopStyleColor();
            ImGui::Separator();
        }
        {
            ImGui::Text("Warnings");
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.5f, 1.0f));
            ImGui::TextUnformatted(std::static_pointer_cast<util::ImGuiLog>(util::Log::instance())->textBuffer(util::Log::Type::kWarning).begin());
            ImGui::PopStyleColor();
        }
        {
            ImGui::Text("Errors");
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.5f, 0.5f, 1.0f));
            ImGui::TextUnformatted(std::static_pointer_cast<util::ImGuiLog>(util::Log::instance())->textBuffer(util::Log::Type::kError).begin());
            ImGui::PopStyleColor();
        }
        ImGui::SetScrollHere(1.0f);
        ImGui::EndChild();
        ImGui::Separator();
    }

    ImGui::End();

    if (m_visualizeSequenceData) {
        ImGui::Begin("Samples");
        static int sequenceIndex = 0;
        static int prefixCount = m_renderOptions.maxRenderPasses;
        static bool showApertureSamples = false;
        bool apertureOptionChanged = false;
        if (ImGui::Checkbox("Show Aperture Samples", &showApertureSamples)) {
            apertureOptionChanged = true;
        }
        bool sequenceIndexMoved = ImGui::SliderInt("Sequence Index", &sequenceIndex, 0, PassGenerator::kNumRandomSequences - 1);
        bool prefixCountMoved = ImGui::SliderInt("Prefix Count", &prefixCount, 1, m_renderOptions.maxRenderPasses);
        if (sequenceIndexMoved || prefixCountMoved || apertureOptionChanged) {
            generateSequenceVisualizationData(sequenceIndex, prefixCount, showApertureSamples);
        }
        ImVec2 windowMin = ImGui::GetWindowContentRegionMin();
        ImVec2 windowMax = ImGui::GetWindowContentRegionMax();
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 topLeft = ImVec2(windowMin.x + windowPos.x, windowMin.y + windowPos.y + 30); // +20 to not overlap with the sequence index slider. This is just a hack.
        ImVec2 bottomRight = ImVec2(windowMax.x + windowPos.x, windowMax.y + windowPos.y);
        // We want to start with 0,0 being the lower left.
        ImVec2 start = ImVec2(topLeft.x, bottomRight.y);
        ImVec2 end = ImVec2(bottomRight.x, topLeft.y);
        ImVec2 diff = ImVec2(end.x - start.x, end.y - start.y);
        for (glm::vec2& v : m_sequenceVisualizationData) {
            ImVec2 point = ImVec2(start.x + diff.x * v.x, start.y + diff.y * v.y);
            ImGui::GetWindowDrawList()->AddCircle(point, 6.0f, IM_COL32(255, 255, 255, 255));
        }
        ImGui::End();
    }

    ImGui::Render();

    return shouldResetRenderer;
}

void HeatrayRenderer::resetRenderer()
{
    m_renderOptions.resetInternalState = true;
    m_renderOptions.camera.viewMatrix = m_camera.orbitCamera.createViewMatrix();
    m_currentPass = 0;
    m_totalPasses = m_renderOptions.maxRenderPasses * (m_renderOptions.enableInteractiveMode ? m_renderOptions.kInteractiveBlockSize.x * m_renderOptions.kInteractiveBlockSize.y : 1);
    m_totalRenderTime = 0.0f;
}

void FreeImageErrorHandler(FREE_IMAGE_FORMAT fif, const char* message) {
    LOG_ERROR("FreeImage*** ");
    if (fif != FIF_UNKNOWN) {
        LOG_ERROR("%s Format", FreeImage_GetFormatFromFIF(fif));
    }
    LOG_ERROR(message);
    LOG_ERROR("***");
}

void HeatrayRenderer::saveScreenshot()
{
    FreeImage_Initialise();
    FreeImage_SetOutputMessage(FreeImageErrorHandler);
    FIBITMAP* bitmap = nullptr;
    if (m_hdrScreenshot) {
        FIBITMAP* hdrBitmap = FreeImage_AllocateT(FIT_RGBAF, m_pixelDimensions.x, m_pixelDimensions.y, 32 * openrl::PixelPackBuffer::kNumChannels); // 32 bits per pixel (RGBA).
        
        // We have to get the pixels from the display texture directly and get them into the proper format.
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_displayPixelBuffer);
        size_t dataSize = m_pixelDimensions.x * m_pixelDimensions.y * sizeof(float) * openrl::PixelPackBuffer::kNumChannels;
        void* pixels = FreeImage_GetBits(hdrBitmap);
        glGetBufferSubData(GL_PIXEL_UNPACK_BUFFER, 0, dataSize, pixels);

        // Convert each pixel to the proper RGB value (Alpha stores the number of passes performed).
        for (uint32_t y = 0; y < FreeImage_GetHeight(hdrBitmap); ++y) {
            float* bits = (float *)FreeImage_GetScanLine(hdrBitmap, y);
            for (uint32_t x = 0; x < FreeImage_GetWidth(hdrBitmap); ++x) {
                float divisor = 1.0f / bits[FI_RGBA_ALPHA];

                bits[FI_RGBA_RED]   *= divisor;
                bits[FI_RGBA_GREEN] *= divisor;
                bits[FI_RGBA_BLUE]  *= divisor;
 
                bits += openrl::PixelPackBuffer::kNumChannels;
            }
        }

        bitmap = FreeImage_ConvertToRGBF(hdrBitmap);
    } else {
        int scale = 1;
#if defined(__APPLE__)
        // Account for retina scaling.
        scale = 2;
#endif // defined(__APPLE__)
        bitmap = FreeImage_AllocateT(FIT_BITMAP, m_pixelDimensions.x * scale, m_pixelDimensions.y * scale, 24); // 8 bits per channel.
        void* pixelData = FreeImage_GetBits(bitmap);
        glReadPixels(UI_WINDOW_WIDTH * scale, 0, m_pixelDimensions.x * scale, m_pixelDimensions.y * scale, GL_BGR, GL_UNSIGNED_BYTE, pixelData);
    }

    FreeImage_Save(FreeImage_GetFIFFromFilename(m_screenshotPath.c_str()), bitmap, m_screenshotPath.c_str(), 0);
    FreeImage_DeInitialise();
    m_shouldSaveScreenshot = false;
}
