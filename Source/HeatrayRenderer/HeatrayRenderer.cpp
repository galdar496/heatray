#include "HeatrayRenderer.h"

#include "Materials/GlassMaterial.h"
#include "Materials/ShowNormalsMaterial.h"
#include "MeshProviders/AssimpMeshProvider.h"
#include "MeshProviders/PlaneMeshProvider.h"
#include "MeshProviders/SphereMeshProvider.h"

#include "Utility/FileDialog.h"
#include "Utility/ImGuiLog.h"
#include <Utility/Random.h>

#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/constants.hpp>
#include "imgui/imgui.h"
#include <FreeImage/FreeImage.h>

#include <assert.h>

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
        #if defined(_WIN32) || defined(_WIN64)
        glewInit();
        #endif

        // Create the pixel-pack buffer and gl texture to use for displaying the raytraced result.
        glEnable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);
        glDisable(GL_BLEND);

        glGenBuffers(1, &m_displayPixelBuffer);
        glGenTextures(1, &m_displayTexture);

        // Setup the texture state.
        glBindTexture(GL_TEXTURE_2D, m_displayTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);

        resizeGLData();

        m_displayProgram.init();

        // Make sure that OpenGL doesn't automatically clamp our display texture. This is because the raytraced image
		// that will be stored in it is actually an accumulation of every ray that has gone through a given pixel.
		// Therefore each pixel value will quickly move beyond 1.0.  Only enabled on Windows as it doesn't seem to be
        // available on macOS.
        #if defined(_WIN32) || defined(_WIN64)
        glClampColorARB(GL_CLAMP_FRAGMENT_COLOR, GL_FALSE);
        #endif
    }

    m_renderOptions.camera.aspectRatio = static_cast<float>(m_renderWindowParams.width) / static_cast<float>(m_renderWindowParams.height);
    m_renderOptions.camera.viewMatrix = m_camera.orbitCamera.createViewMatrix();
	m_renderOptions.camera.setApertureRadius();

    // Load the default scene.
    changeScene(m_renderOptions.scene);

    return true;
}

void HeatrayRenderer::destroy()
{
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

void HeatrayRenderer::changeScene(std::string const& sceneName)
{
	m_groundPlane.reset();
	m_editableMaterialScene.active = false;

    if (sceneName == "Editable Material") {
		LOG_INFO("Loading scene: %s", sceneName.c_str());
        m_renderer.loadScene([this](RLMesh::SetupSystemBindingsCallback systemSetupCallback) {
            for (auto mesh : m_sceneData) {
                mesh.destroy();
            }
            m_sceneData.clear();
			m_renderOptions.camera.focusDistance = m_camera.orbitCamera.distance; // Auto-focus to the center of the scene.

            SphereMeshProvider sphereMeshProvider(50, 50, 1.0f);
            std::shared_ptr<PhysicallyBasedMaterial> material = std::make_shared<PhysicallyBasedMaterial>();
            PhysicallyBasedMaterial::Parameters params;
            params.metallic = 0.0f;
            params.roughness = 0.0f;
            params.baseColor = glm::vec3(1.0f);
            params.specularF0 = 0.5f;
			params.clearCoat = 0.0f;
			params.clearCoatRoughness = 0.0f;
            material->build(params);     
            m_sceneData.push_back(RLMesh(&sphereMeshProvider, { material }, systemSetupCallback, glm::mat4(1.0f)));

			m_editableMaterialScene.material = material;
			m_editableMaterialScene.materialParams = params;
			m_editableMaterialScene.active = true;
        });
    } else if (sceneName == "Multi-Material") {
		LOG_INFO("Loading scene: %s", sceneName.c_str());
        m_renderer.loadScene([this](RLMesh::SetupSystemBindingsCallback systemSetupCallback) {
            for (auto mesh : m_sceneData) {
                mesh.destroy();
            }
            m_sceneData.clear();
			m_renderOptions.camera.focusDistance = m_camera.orbitCamera.distance; // Auto-focus to the center of the scene.

            PlaneMeshProvider planeMeshProvider(15, 15);
            glm::vec3 xAxis = glm::vec3(1.0f, 0.0f, 0.0f);
            glm::vec3 yAxis = glm::vec3(0.0f, 1.0f, 0.0f);
            glm::vec3 zAxis = glm::vec3(0.0f, 0.0f, 1.0f);

            // Bottom plane.
            {
				std::shared_ptr<PhysicallyBasedMaterial> material = std::make_shared<PhysicallyBasedMaterial>();
                PhysicallyBasedMaterial::Parameters params;
                params.metallic = 0.0f;
                params.roughness = 1.0f;
                params.baseColor = glm::vec3(0.9f);
                params.specularF0 = 0.0f;
                material->build(params);
                glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.5f, 0.0f));
                m_sceneData.push_back(RLMesh(&planeMeshProvider, { material }, systemSetupCallback, translation));
            }
#if 0
            // Right plane.
            {
				std::shared_ptr<PhysicallyBasedMaterial> material = std::make_shared<PhysicallyBasedMaterial>();
                PhysicallyBasedMaterial::Parameters params;
                params.metallic = 0.0f;
				params.roughness = 1.0f;
				params.baseColor = glm::vec3(0.9f);
				params.specularF0 = 0.0f;
                material->build(params);
                glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::half_pi<float>(), zAxis);
                glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(2.5f, 1.0f, 0.0f));
                m_sceneData.push_back(RLMesh(&planeMeshProvider, { material }, systemSetupCallback, translation * rotation));
            }

            // Back plane.
            {
				std::shared_ptr<PhysicallyBasedMaterial> material = std::make_shared<PhysicallyBasedMaterial>();
                PhysicallyBasedMaterial::Parameters params;
				params.metallic = 0.0f;
				params.roughness = 1.0f;
				params.baseColor = glm::vec3(0.9f);
				params.specularF0 = 0.0f;
                material->build(params);
                glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::half_pi<float>(), xAxis);
                glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, -2.5f));
                m_sceneData.push_back(RLMesh(&planeMeshProvider, { material }, systemSetupCallback, translation * rotation));
            }

            // Left plane.
            /*{
                std::shared_ptr<PhysicallyBasedMaterial> material = std::make_shared<PhysicallyBasedMaterial>();
                PhysicallyBasedMaterial::Parameters params;
                params.metallic = 0.0f;
                params.roughness = 0.8f;
                params.baseColor = glm::vec3(0.0f, 0.8f, 0.0f);
                params.specularF0 = 0.5f;
                material->build(params);
                glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::half_pi<float>() * 3.0f, zAxis);
                glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(-2.5f, 1.0f, 0.0f));
                m_sceneData.push_back(RLMesh(&planeMeshProvider, { material }, systemSetupCallback, translation * rotation));
            }*/
#endif

            float radius = 1.0f;
            SphereMeshProvider sphereMeshProvider(50, 50, radius);

            // Sphere 1.
            {
				std::shared_ptr<PhysicallyBasedMaterial> material = std::make_shared<PhysicallyBasedMaterial>();
                PhysicallyBasedMaterial::Parameters params;
				params.metallic = 1.0f;
				params.roughness = 0.1f;
				params.baseColor = glm::vec3(0.4f);
				params.specularF0 = 0.3f;
                material->build(params);
                glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(-0.9f, -0.5f, -0.8f));
                m_sceneData.push_back(RLMesh(&sphereMeshProvider, { material }, systemSetupCallback, translation));
            }

            // Sphere 2.
            {
				std::shared_ptr<GlassMaterial> material = std::make_shared<GlassMaterial>();
                GlassMaterial::Parameters params;
				params.roughness = 0.1f;
				params.baseColor = glm::vec3(0.9f, 0.6f, 0.6f);
				params.ior = 1.57f;
				params.density = 0.5f;
                material->build(params);
                glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(1.2f, -0.5f, 0.8f));
                m_sceneData.push_back(RLMesh(&sphereMeshProvider, { material }, systemSetupCallback, translation));
            }
        });
    } else if (sceneName == "Sphere Array") {
		LOG_INFO("Loading scene: %s", sceneName.c_str());
        m_renderer.loadScene([this](RLMesh::SetupSystemBindingsCallback systemSetupCallback) {
            for (auto mesh : m_sceneData) {
                mesh.destroy();
            }
            m_sceneData.clear();
			m_renderOptions.camera.focusDistance = m_camera.orbitCamera.distance; // Auto-focus to the center of the scene.

            // The default scene is a plane with various spheres.
            {
                PlaneMeshProvider planeMeshProvider(25, 25);

				std::shared_ptr<PhysicallyBasedMaterial> material = std::make_shared<PhysicallyBasedMaterial>();
                PhysicallyBasedMaterial::Parameters params;
                params.metallic = 0.0f;
                params.roughness = 1.0f;
                params.baseColor = glm::vec3(0.6f);
                params.specularF0 = 0.5f;
                material->build(params); 
                glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.5f, 0.0f));
                m_sceneData.push_back(RLMesh(&planeMeshProvider, { material }, systemSetupCallback, translation));
            }

            float radius = 0.5f;
            SphereMeshProvider sphereMeshProvider(50, 50, radius);
            float roughness = 0.0f;
            float padding = radius * 0.2f;
            float startX = (-5.0f * (radius * 2.0f + padding)) + ((radius * 2.0f + padding) * 0.5f);
            
            // Non-metals.
            {
                for (int iSphere = 0; iSphere < 10; ++iSphere) {
					std::shared_ptr<PhysicallyBasedMaterial> material = std::make_shared<PhysicallyBasedMaterial>();
                    PhysicallyBasedMaterial::Parameters params;
                    params.metallic = 0.0f;
                    params.roughness = roughness;
                    params.baseColor = glm::vec3(1.0f);
                    params.specularF0 = 1.0f;
                    material->build(params);
                    glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(startX, 0.0f, 0.0f));
                    m_sceneData.push_back(RLMesh(&sphereMeshProvider, { material }, systemSetupCallback, translation));

                    roughness += 0.1f;
                    startX += radius * 2.0f + padding;
                }
            }

            // Metals.
            {
                roughness = 0.0f;
                startX = (-5.0f * (radius * 2.0f + padding)) + ((radius * 2.0f + padding) * 0.5f);
                for (int iSphere = 0; iSphere < 10; ++iSphere) {
					std::shared_ptr<PhysicallyBasedMaterial> material = std::make_shared<PhysicallyBasedMaterial>();
                    PhysicallyBasedMaterial::Parameters params;
                    params.metallic = 1.0f;
                    params.roughness = roughness;
                    params.baseColor = glm::vec3(1.0f);
                    params.specularF0 = 0.5f;
                    material->build(params);
                    glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(startX, 1.5f, 0.0f));
                    m_sceneData.push_back(RLMesh(&sphereMeshProvider, { material }, systemSetupCallback, translation));

                    roughness += 0.1f;
                    startX += radius * 2.0f + padding;
                }
            }
        });
    } else {
        m_renderer.loadScene([this, sceneName](RLMesh::SetupSystemBindingsCallback systemSetupCallback) {
            for (auto mesh : m_sceneData) {
                mesh.destroy();
            }
            m_sceneData.clear();

			LOG_INFO("Loading scene: %s", sceneName.c_str());
            AssimpMeshProvider assimpMeshProvider(sceneName, (m_scene_units == SceneUnits::kCentimeters), m_swapYZ);

            const std::vector<std::shared_ptr<Material>> &materials = assimpMeshProvider.GetMaterials();
            
            m_sceneData.push_back(RLMesh(&assimpMeshProvider, materials, systemSetupCallback, glm::mat4(1.0f)));
			m_sceneAABB = assimpMeshProvider.sceneAABB();
			m_camera.orbitCamera.target = m_sceneAABB.center();
			m_camera.orbitCamera.distance = m_sceneAABB.radius() * 3.0f; // Add some extra scale.
			m_camera.orbitCamera.max_distance = m_sceneAABB.radius() * 10.0f;
			m_renderOptions.camera.viewMatrix = m_camera.orbitCamera.createViewMatrix();

			m_distanceScale = m_sceneAABB.radius();

			m_renderOptions.camera.focusDistance = m_camera.orbitCamera.distance; // Auto-focus to the center of the scene.

			resetRenderer();
        });
    }
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
		// Shift the quad to account for the UI.
		float start = ((float(UI_WINDOW_WIDTH) / float(m_windowParams.width)) * 2.0f) - 1.0f;

        m_displayProgram.bind(0, m_post_processing_params); 
        glBindTexture(GL_TEXTURE_2D, m_displayTexture);
        glBegin(GL_QUADS);
			glTexCoord2d(0.0, 0.0); glVertex2f(start, -1.0f);
			glTexCoord2d(1.0, 0.0); glVertex2f(1.0f, -1.0f);
			glTexCoord2d(1.0, 1.0); glVertex2f(1.0f, 1.0f);
			glTexCoord2d(0.0, 1.0); glVertex2f(start, 1.0f);
        glEnd();
        glBindTexture(GL_TEXTURE_2D, 0);
        m_displayProgram.unbind();
    }

    // If requested to do a screenshot, perform it now.
    if (m_shouldSaveScreenshot) {
        saveScreenshot();
    }

    m_resetRequested |= renderUI() | m_cameraUpdated;
	m_cameraUpdated = false;

    // Kick the raytracer if necessary.
    if (!m_justResized && // Don't kick the renderer if we've just resized.
        (!m_renderingFrame && (m_currentPass < m_totalPasses)) || // If we haven't yet rendered all passes and are not currently rendering a pass.
        (m_resetRequested && !m_renderingFrame)) { // If we've been asked to reset but are not in the middle of another frame.

        if (m_resetRequested) {
            resetRenderer();
        }

        m_renderingFrame = true;
        m_shouldCopyPixels.store(false);
        // Tell the pathtracer to start generating a new frame.
        m_renderer.renderPass(m_renderOptions,
            [this](const openrl::PixelPackBuffer& results, float passTime)
            { 
                const float* pixelData = results.mapPixelData();
                m_pathracedPixels.store(pixelData);
                m_pixelDimensions = glm::ivec2(results.width(), results.height());
                m_shouldCopyPixels.store(true);
                m_renderingFrame = false;
                m_currentPassTime = passTime;
                m_totalRenderTime += passTime;
                ++m_currentPass;
            });

        m_renderOptions.resetInternalState = false;
        m_resetRequested = false;
    }

    m_justResized = false;
}

void HeatrayRenderer::handlePendingFileLoads()
{
    if (m_userRequestedFileLoad) {
        std::vector<std::string> filenames = util::OpenFileDialog();

        if (filenames.size() > 0) {
            changeScene(filenames[0]);
            resetRenderer();
        }

        m_userRequestedFileLoad = false;
    }
}

void HeatrayRenderer::adjustCamera(const float phi_delta, const float theta_delta, const float distance_delta)
{
	if (m_renderOptions.enableInteractiveMode) {
		float SCALE = 0.5f;

		m_camera.orbitCamera.phi += glm::radians(phi_delta) * SCALE;
		m_camera.orbitCamera.theta += glm::radians(theta_delta) * SCALE;
		m_camera.orbitCamera.distance += distance_delta * SCALE * m_distanceScale;

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
		}

		m_renderOptions.camera.viewMatrix = m_camera.orbitCamera.createViewMatrix();
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
				util::uniformRandomFloats<glm::vec3>(&m_sequenceVisualizationData[0], renderPasses, sequenceIndex, 0.0f, 1.0f);
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

void HeatrayRenderer::renderMaterialEditor(std::shared_ptr<PhysicallyBasedMaterial> material, PhysicallyBasedMaterial::Parameters& parameters)
{
	bool materialChanged = false;

	if (ImGui::SliderFloat3("BaseColor", parameters.baseColor.data.data, 0.0f, 1.0f)) {
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

	if (materialChanged) {
		m_renderer.runOpenRLTask([this, material, &parameters]() {
			material->modify(parameters);
			resetRenderer();
		});
	}
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
            if (ImGui::SliderInt("Max ray depth", &m_renderOptions.maxRayDepth, 1, 15)) {
                shouldResetRenderer = true;
            }
            if (ImGui::Checkbox("Enable interactive mode", &m_renderOptions.enableInteractiveMode)) {
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
        static const char* options[] = { "studio.hdr", "glacier.exr", "uffizi.exr", "bridge.hdr", "arches.hdr", "white furnace test", "custom..." };
		static constexpr size_t NUM_OPTIONS = sizeof(options) / sizeof(options[0]);
		static constexpr size_t CUSTOM_OPTION_INDEX = NUM_OPTIONS - 1;


        static size_t currentSelection = 0;
        if (ImGui::BeginCombo("Environment map", options[currentSelection])) {
            for (size_t iOption = 0; iOption < NUM_OPTIONS; ++iOption) {
                bool isSelected = (currentSelection == iOption);
                if (ImGui::Selectable(options[iOption], false)) {
                    currentSelection = iOption;
					if (currentSelection != CUSTOM_OPTION_INDEX) {
						m_renderOptions.environment.map = std::string(options[iOption]);
						m_renderOptions.environment.builtInMap = true;
					} else {
						std::vector<std::string> filenames = util::OpenFileDialog();

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

		if (ImGui::SliderAngle("Environment Rotation", &m_renderOptions.environment.thetaRotation, 0.0f, 360.0f)) {
			shouldResetRenderer = true;
		}

		if (ImGui::SliderFloat("Environment Exposure", &m_renderOptions.environment.exposureCompensation, -10.0f, 10.0f)) {
			shouldResetRenderer = true;
		}
    }
    if (ImGui::CollapsingHeader("Scene options")) {
        if (ImGui::Button("Load Scene...")) {
            m_userRequestedFileLoad = true;
        }
		ImGui::Text("Scene Units:");
		if (ImGui::RadioButton("Meters", m_scene_units == SceneUnits::kMeters)) {
			m_scene_units = SceneUnits::kMeters;
		}
		if (ImGui::RadioButton("Centimeters", m_scene_units == SceneUnits::kCentimeters)) {
			m_scene_units = SceneUnits::kCentimeters;
		}
        ImGui::Checkbox("Swap Y & Z on load", &m_swapYZ);

        static const char* options[] = { "Sphere Array", "Multi-Material", "Editable Material" };

        static unsigned int currentSelection = 1;
        if (ImGui::BeginCombo("Built-In Scenes", options[currentSelection])) {
            for (int iOption = 0; iOption < sizeof(options) / sizeof(options[0]); ++iOption) {
                bool isSelected = currentSelection == iOption;
                if (ImGui::Selectable(options[iOption], false)) {
                    currentSelection = iOption;
                    m_renderOptions.scene = options[iOption];
                    changeScene(m_renderOptions.scene);
                    shouldResetRenderer = true;
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

		bool groundPlaneExists = (m_groundPlane.mesh != nullptr);

		if (ImGui::Button(groundPlaneExists ? "Remove Ground Plane" : "Add Ground Plane")) {
			m_renderer.loadScene([this, groundPlaneExists](RLMesh::SetupSystemBindingsCallback systemSetupCallback) {
				if (groundPlaneExists) {
					auto groundRLmesh = m_sceneData.begin() + m_groundPlane.meshIndex;
					groundRLmesh->destroy();
					m_sceneData.erase(groundRLmesh);
					m_groundPlane.reset();
				} else {
					size_t planeSize = size_t(m_sceneAABB.radius()) * 5;
					PlaneMeshProvider planeMeshProvider(planeSize, planeSize);

					std::shared_ptr<PhysicallyBasedMaterial> material = std::make_shared<PhysicallyBasedMaterial>();
					m_groundPlane.materialParams.metallic = 0.0f;
					m_groundPlane.materialParams.roughness = 0.9f;
					m_groundPlane.materialParams.baseColor = glm::vec3(0.9f);
					m_groundPlane.materialParams.specularF0 = 0.2f;
					material->build(m_groundPlane.materialParams);
					glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, m_sceneAABB.min.y, 0.0f));
					m_sceneData.push_back(RLMesh(&planeMeshProvider, { material }, systemSetupCallback, translation));

					m_groundPlane.material = material;
					m_groundPlane.meshIndex = m_sceneData.size() - 1;
					m_groundPlane.mesh = &(m_sceneData[m_groundPlane.meshIndex]);
				}

				resetRenderer();
			});
		}

		if (m_groundPlane.mesh) {
			ImGui::Text("Ground Material");
			renderMaterialEditor(m_groundPlane.material, m_groundPlane.materialParams);
		}
    }
    if (ImGui::CollapsingHeader("Camera options")) {
        ImGui::Text("Orbital Camera");
        bool changed = ImGui::SliderAngle("Phi", &(m_camera.orbitCamera.phi), 0.0f, 360.0f);
        changed |= ImGui::SliderAngle("Theta", &(m_camera.orbitCamera.theta), -90.0f, 90.0f);
        changed |= ImGui::SliderFloat("Distance", &(m_camera.orbitCamera.distance), 0.0f, m_camera.orbitCamera.max_distance);
        if (changed) {
            m_renderOptions.camera.viewMatrix = m_camera.orbitCamera.createViewMatrix();
            shouldResetRenderer = true;
        }

        ImGui::Text("Lens Properties");
        if (ImGui::SliderFloat("Focus distance(m)", &m_renderOptions.camera.focusDistance, 0.001f, 40.0f)) {
            shouldResetRenderer = true;
        }
        if (ImGui::SliderFloat("Focal length(mm)", &m_renderOptions.camera.focalLength, 10.0f, 200.0f)) {
			m_renderOptions.camera.setApertureRadius();
            shouldResetRenderer = true;
        }
		{
			static unsigned int currentSelection = 0;
			static const char* options[PassGenerator::RenderOptions::Camera::NUM_FSTOPS] = { 
				"f32", "f22", "f16", "f11", "f8", "f5.6", "f4", "f2.8", "f2", "f1.4", "f1"
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
    if (ImGui::CollapsingHeader("Post processing")) {
        ImGui::Checkbox("ACES tonemapping enabled", &m_post_processing_params.tonemapping_enabled);
        ImGui::SliderFloat("Exposure compensation", &m_post_processing_params.exposure, -10.0f, 10.0f);
		ImGui::SliderFloat("Brightness", &m_post_processing_params.brightness, -1.0f, 1.0f);
		ImGui::SliderFloat("Contrast", &m_post_processing_params.contrast, -1.0f, 1.0f);
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
        if (ImGui::Button("Save")) {
			std::vector<std::string> names = util::SaveFileDialog(m_hdrScreenshot ? "tiff" : "png");
			if (!names.empty()) {
				m_screenshotPath = names[0];
				m_shouldSaveScreenshot = true; // Do this on the next frame before drawing UI.
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
        for (glm::vec3& v : m_sequenceVisualizationData) {
            ImVec2 point = ImVec2(start.x + diff.x * v.x, start.y + diff.y * v.y);
            ImGui::GetWindowDrawList()->AddCircle(point, 6.0f, IM_COL32(255, 255, 255, 255));
        }
        ImGui::End();
    }

	if (m_editableMaterialScene.active) {
		ImGui::Begin("PBR Material");
		renderMaterialEditor(m_editableMaterialScene.material, m_editableMaterialScene.materialParams);
		ImGui::End();
	}

    ImGui::Render();

    return shouldResetRenderer;
}

void HeatrayRenderer::resetRenderer()
{
    m_renderOptions.resetInternalState = true;
    m_currentPass = 0;
    m_totalPasses = m_renderOptions.maxRenderPasses * (m_renderOptions.enableInteractiveMode ? m_renderOptions.kInteractiveBlockSize.x * m_renderOptions.kInteractiveBlockSize.y : 1);
    m_totalRenderTime = 0.0f;
}

void FreeImageErrorHandler(FREE_IMAGE_FORMAT fif, const char* message) {
    printf("\n*** ");
    if (fif != FIF_UNKNOWN) {
        printf("%s Format\n", FreeImage_GetFormatFromFIF(fif));
    }
    printf(message);
    printf(" ***\n");
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
        bitmap = FreeImage_AllocateT(FIT_BITMAP, m_pixelDimensions.x, m_pixelDimensions.y, 24); // 8 bits per channel.
        void* pixelData = FreeImage_GetBits(bitmap);
        glReadPixels(UI_WINDOW_WIDTH, 0, m_pixelDimensions.x, m_pixelDimensions.y, GL_BGR, GL_UNSIGNED_BYTE, pixelData);
    }

    FreeImage_Save(FreeImage_GetFIFFromFilename(m_screenshotPath.c_str()), bitmap, m_screenshotPath.c_str(), 0);
    FreeImage_DeInitialise();
    m_shouldSaveScreenshot = false;
}
