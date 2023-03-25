//
//  HeatrayRenderer.cpp
//  Heatray5 macOS
//
//  Created by Cody White on 3/18/23.
//

#include "HeatrayRenderer_macOS.hpp"

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
    
    m_commandQueue = m_device->newCommandQueue();
    
    m_renderWindowParams.width = renderWidth - UI_WINDOW_WIDTH;
    m_renderWindowParams.height = renderHeight;
    m_passGenerator.init(device, m_renderWindowParams.width, m_renderWindowParams.height);
    
    imgui_helper::init_imgui(device, static_cast<void*>(view));
    
    // Install the imgui log now that imgui has been setup.
    util::ImGuiLog::install();
    
    LOG_INFO("Huz");
    LOG_WARNING("Huzzah!");
    LOG_ERROR("Oh no!");
    
    return true;
}

void HeatrayRenderer::destroy() {
    m_passGenerator.destroy();
    
    m_commandQueue->release();
    m_device->release();
}

void HeatrayRenderer::resize(const uint32_t newWidth, const uint32_t newHeight) {
    m_passGenerator.resize(m_device, newWidth, newHeight);
}

void HeatrayRenderer::render(MTK::View* view) {
    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init(); // Do we really need this?
    
    MTL::CommandBuffer *commandBuffer = m_commandQueue->commandBuffer();
    MTL::RenderPassDescriptor *descriptor = view->currentRenderPassDescriptor();
    MTL::RenderCommandEncoder *encoder = commandBuffer->renderCommandEncoder(descriptor);
    encoder->endEncoding();
    
    renderUI(view, commandBuffer);
    
    commandBuffer->presentDrawable(view->currentDrawable());
    commandBuffer->commit();
    
    pool->release();
}

void HeatrayRenderer::adjustCamera(const float phiDelta, const float thetaDelta, const float distanceDelta) {
    
}

void HeatrayRenderer::resetRenderer() {
    
}

bool HeatrayRenderer::renderUI(MTK::View* view, MTL::CommandBuffer* cmdBuffer) {
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
    
    imgui_helper::end_imgui_frame(static_cast<void*>(view), cmdBuffer);
    
    return false;
}
