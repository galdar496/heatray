//
//  HeatrayRenderer.cpp
//  Heatray5 macOS
//
//  Created by Cody White on 3/18/23.
//

#include "HeatrayRenderer_macOS.hpp"

#include "Shaders/DisplayShaderTypes.h"
#include "Utility/FileDialog.h"
#include "Utility/ImGuiLog.h"

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
    
    setupDisplayShader(view);
    
    // Setup window-size specific data.
    resize(renderWidth, renderHeight);
    
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
    encodeUI(view, commandBuffer, encoder);
    encoder->endEncoding();
    
    commandBuffer->presentDrawable(view->currentDrawable());
    commandBuffer->commit();
    
    pool->release();
}

void HeatrayRenderer::adjustCamera(const float phiDelta, const float thetaDelta, const float distanceDelta) {
    
}

void HeatrayRenderer::resetRenderer() {
    
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
    imgui_helper::start_imgui_frame(static_cast<void*>(view));
    
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(UI_WINDOW_WIDTH, static_cast<float>(m_windowParams.height)));
    bool shouldCloseWindow = false;
    ImGui::Begin("Main Menu", &shouldCloseWindow, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);
    
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
    
    return false;
}
