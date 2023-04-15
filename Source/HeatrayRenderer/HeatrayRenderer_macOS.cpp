//
//  HeatrayRenderer.cpp
//  Heatray5 macOS
//
//  Created by Cody White on 3/18/23.
//

#include "HeatrayRenderer_macOS.hpp"

#include "Materials/PhysicallyBasedMaterial.h"
#include "Scene/SphereMeshProvider.h"
#include "Shaders/DisplayShaderTypes.h"
#include "Utility/FileDialog.h"
#include "Utility/ImGuiLog.h"
#include "Utility/Math.h"

#include "imgui_helper.hpp"

#include <Metal/MTLCommandBuffer.hpp>
#include <Metal/MTLRenderPass.hpp>
#include <Metal/MTLRenderCommandEncoder.hpp>

#include <fstream>

bool HeatrayRenderer::init(MTL::Device* device, MTK::View* view, const uint32_t renderWidth, const uint32_t renderHeight) {
    // Ensure the device remains valid.
    device->retain();
    m_device = device;
    
    // Imgui setup.
    {
        imgui_helper::init_imgui(device, static_cast<void*>(view));
        
        // Install the imgui log now that imgui has been setup.
        util::ImGuiLog::install();
    }
    
    LOG_INFO("Initialized Metal device \"%s\"", m_device->name()->cString(NS::UTF8StringEncoding));
    
    m_shaderLibrary = m_device->newDefaultLibrary();
    m_commandQueue = m_device->newCommandQueue();
    
    m_passGenerator.init(device, m_shaderLibrary);
    m_passGenerator.installStatUpdateCallback([this](PassGenerator::RenderStats stats) {
        m_currentPassTime = stats.passGPUTimeSeconds;
        m_totalRenderTime += stats.passGPUTimeSeconds;
    });
    
    setupDisplayShader(view);
    
    // Setup window-size specific data.
    resize(renderWidth, renderHeight);
    
    // Setup some defaults.
    m_renderOptions.scene = "Test Sphere";
    m_renderOptions.camera.viewMatrix = m_camera.orbitCamera.createViewMatrix();
    
    // Load the default scene.
    m_scene = Scene::create();
    changeScene(m_renderOptions.scene, true);
    
    // Get the internal state ready to go.
    resetRenderer();
    
    return true;
}

void HeatrayRenderer::destroy() {
    m_passGenerator.destroy();
    
    m_display.pipelineState->release();
    m_display.vertexConstants->release();
    
    m_commandQueue->release();
    m_device->release();
}

void HeatrayRenderer::resize(const uint32_t newWidth, const uint32_t newHeight) {
    m_renderWindowParams.width = newWidth - UI_WINDOW_WIDTH;
    m_renderWindowParams.height = newHeight;
    
    m_passGenerator.resize(m_device, m_renderWindowParams.width, m_renderWindowParams.height);
    
    // Update the display shader constants which are based on the frame size
    // to avoid rendering into the part of the view containing the UI.
    {
        DisplayVertexShader::Constants* constants = static_cast<DisplayVertexShader::Constants*>(m_display.vertexConstants->contents());
        constants->quadOffset = ((float(UI_WINDOW_WIDTH) / float(newWidth)) * 2.0f) - 1.0f;
    }
    
    resetRenderer();
}

void HeatrayRenderer::render(MTK::View* view) {
    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init(); // Do we really need this?
    
    MTL::CommandBuffer *commandBuffer = m_commandQueue->commandBuffer();
    
    // Encode the commands to generate a single pass of the pathtracer.
    MTL::Texture* raytracedPixels = m_passGenerator.encodePass(commandBuffer, m_renderOptions);
    
    // Encode the commands to visualize the current state of the pathtracer
    // as well as the UI.
    MTL::RenderPassDescriptor *descriptor = view->currentRenderPassDescriptor();
    MTL::RenderCommandEncoder *encoder = commandBuffer->renderCommandEncoder(descriptor);
    encodeDisplay(view, encoder, raytracedPixels);
    if (encodeUI(view, commandBuffer, encoder)) {
        resetRenderer();
    }
    encoder->endEncoding();
    
    commandBuffer->presentDrawable(view->currentDrawable());
    commandBuffer->commit();
    
    pool->release();
}

void HeatrayRenderer::adjustCamera(const float phiDelta, const float thetaDelta, const float distanceDelta) {
    if (!m_camera.locked) {
        float SCALE = 0.5f;

        m_camera.orbitCamera.phi += util::radians(phiDelta) * SCALE;
        m_camera.orbitCamera.theta += util::radians(thetaDelta) * SCALE;
        m_camera.orbitCamera.distance += distanceDelta * SCALE * m_distanceScale;

        // Ensure that the camera parameters are valid.
        {
            if (m_camera.orbitCamera.phi < 0.0f) {
                m_camera.orbitCamera.phi += util::constants::TWO_PI;
            }
            else if (m_camera.orbitCamera.phi > util::constants::TWO_PI) {
                m_camera.orbitCamera.phi -= util::constants::TWO_PI;
            }

            m_camera.orbitCamera.theta = std::clamp(m_camera.orbitCamera.theta, -util::constants::HALF_PI, util::constants::HALF_PI);
            m_camera.orbitCamera.distance = std::clamp(m_camera.orbitCamera.distance, 0.0f, m_camera.orbitCamera.max_distance);

            m_renderOptions.camera.focusDistance = m_camera.orbitCamera.distance; // Auto-focus to the center of the scene.
        }
    }
}

void HeatrayRenderer::resetRenderer() {
    m_renderOptions.resetInternalState = true;
    m_renderOptions.camera.viewMatrix = m_camera.orbitCamera.createViewMatrix();
    m_currentPass = 0;
    m_totalRenderTime = 0.0f;
}

void HeatrayRenderer::setupDisplayShader(const MTK::View* view) {
    // Setup the pipeline that will be used to visualize the raytraced data.
    {
        MTL::Function* vertexFunction = m_shaderLibrary->newFunction(NS::String::string("DisplayVS", NS::UTF8StringEncoding));
        MTL::Function* fragmentFunction = m_shaderLibrary->newFunction(NS::String::string("DisplayFS", NS::UTF8StringEncoding));
        
        MTL::RenderPipelineDescriptor* descriptor = MTL::RenderPipelineDescriptor::alloc()->init();
        descriptor->setVertexFunction(vertexFunction);
        descriptor->setFragmentFunction(fragmentFunction);
        descriptor->colorAttachments()->object(0)->setPixelFormat(view->colorPixelFormat());
        
        NS::Error* error = nullptr;
        m_display.pipelineState = m_device->newRenderPipelineState(descriptor, &error);
        if (!m_display.pipelineState) {
            LOG_ERROR("Failed to create display pipeline: \"%s\"", error->localizedDescription()->utf8String());
            assert(false);
        }
        
        descriptor->release();
    }
    
    // Allocate the buffers used for shader constants.
    m_display.vertexConstants = m_device->newBuffer(sizeof(DisplayVertexShader::Constants), MTL::ResourceStorageModeShared);
}

void HeatrayRenderer::encodeDisplay(MTK::View* view, MTL::RenderCommandEncoder* encoder, MTL::Texture* raytracedPixels) {
    encoder->pushDebugGroup(NS::String::string("Display Shader", NS::UTF8StringEncoding));
    encoder->setRenderPipelineState(m_display.pipelineState);
    encoder->setVertexBuffer(m_display.vertexConstants, 0, DisplayVertexShader::BufferLocation);
    
    // Draw a screen-aligned quad which will read from the raytraced pixels and perform an extra processing required.
    encoder->setFragmentTexture(raytracedPixels, 0);
    encoder->drawPrimitives(MTL::PrimitiveTypeTriangleStrip, (NS::UInteger)0, 4);
    
    encoder->popDebugGroup();
}

bool HeatrayRenderer::encodeUI(MTK::View* view, MTL::CommandBuffer* cmdBuffer, MTL::RenderCommandEncoder* encoder) {
    bool shouldResetRenderer = false;
    
    imgui_helper::start_imgui_frame(static_cast<void*>(view));
    
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(UI_WINDOW_WIDTH, static_cast<float>(m_windowParams.height)));
    bool shouldCloseWindow = false;
    ImGui::Begin("Main Menu", &shouldCloseWindow, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);
    
    ImGui::Text("Render stats:");
    {
        ImGui::Text("Passes completed: %u\n", uint32_t(m_currentPass));
        ImGui::Text("Pass time(s): %f\n", m_currentPassTime);
        ImGui::Text("Total render time(s): %f\n", m_totalRenderTime);
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
            changed |= ImGui::InputFloat3("Target", m_camera.orbitCamera.target.uiValue<float>(), "%f", ImGuiInputTextFlags_CharsDecimal);
            if (changed) {
                shouldResetRenderer = true;
            }
        }
    }
    
    // Console log.
    {
        std::shared_ptr<util::ImGuiLog> imguiLog = std::static_pointer_cast<util::ImGuiLog>(util::Log::instance());
        if (imguiLog) {
            ImGui::Separator();
            ImGui::Text("Output Console");
            ImGui::SameLine();
            if (ImGui::Button("Clear")) {
                imguiLog->clear();
            }
            ImGui::SameLine();
            if (ImGui::Button("Write To Disk")) {
                std::vector<std::string> names = util::SaveFileDialog("txt");
                if (!names.empty()) {
                    std::ofstream fout;
                    fout.open(names[0]);
                    if (fout) {
                        bool temp;
                        fout << "INFO:" << std::endl;
                        fout << imguiLog->textBuffer(util::Log::Type::kInfo, temp).c_str() << std::endl;
                        fout << "WARNINGS:" << std::endl;
                        fout << imguiLog->textBuffer(util::Log::Type::kWarning, temp).c_str() << std::endl;
                        fout << "ERRORS:" << std::endl;
                        fout << imguiLog->textBuffer(util::Log::Type::kError, temp).c_str() << std::endl;
                        fout.close();
                    }
                }
            }
            ImGui::Separator();
            bool shouldScrollToBottom = false;
            ImGui::BeginChild("Info Console Log", ImVec2(0.0f, -1.0f), false, ImGuiWindowFlags_AlwaysHorizontalScrollbar);
            {
                ImGui::Text("Info");
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                bool newText = false;
                ImGui::TextUnformatted(imguiLog->textBuffer(util::Log::Type::kInfo, newText).begin());
                shouldScrollToBottom |= newText;
                ImGui::PopStyleColor();
                ImGui::Separator();
            }
            {
                ImGui::Text("Warnings");
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.2f, 1.0f));
                bool newText = false;
                ImGui::TextUnformatted(imguiLog->textBuffer(util::Log::Type::kWarning, newText).begin());
                shouldScrollToBottom |= newText;
                ImGui::PopStyleColor();
                ImGui::Separator();
            }
            {
                ImGui::Text("Errors");
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
                bool newText = false;
                ImGui::TextUnformatted(imguiLog->textBuffer(util::Log::Type::kError, newText).begin());
                shouldScrollToBottom |= newText;
                ImGui::PopStyleColor();
            }
            if (shouldScrollToBottom) {
                ImGui::SetScrollHereY(1.0f);
            }
            ImGui::EndChild();
            ImGui::Separator();
        }
    }
    
    ImGui::End();
    
    imgui_helper::end_imgui_frame(cmdBuffer, encoder);
    
    return shouldResetRenderer;
}

void HeatrayRenderer::changeScene(const std::string &sceneName, const bool moveCamera) {
    m_scene->clearAll();
    LOG_INFO("Loading scene: %s", sceneName.c_str());
    
    if (sceneName == "Test Sphere") {
        SphereMeshProvider sphereMeshProvider(50, 50, 1.0f, "Sphere");
        std::shared_ptr<PhysicallyBasedMaterial> material = std::make_shared<PhysicallyBasedMaterial>("PBR");
        PhysicallyBasedMaterial::Parameters &params = material->parameters();
        params.metallic = 0.0f;
        params.roughness = 1.0f;
        params.baseColor = simd::make_float3(0.8f);
        params.specularF0 = 0.0f;
        params.clearCoat = 0.0f;
        params.clearCoatRoughness = 0.0f;
        params.forceEnableAllTextures = true;
        
        m_scene->addMesh(&sphereMeshProvider, { material }, matrix_identity_float4x4, m_device);
    }
}
