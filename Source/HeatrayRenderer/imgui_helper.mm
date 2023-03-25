//
//  imgui_initializer.cpp
//  Heatray5 macOS
//
//  Created by Cody White on 3/23/23.
//

#include "imgui_helper.hpp"

#include <imgui.h>
#include <backends/imgui_impl_metal.h>
#include <backends/imgui_impl_osx.h>

#include <Metal/Metal.h>
#include <MetalKit/MetalKit.h>

namespace imgui_helper {

void init_imgui(MTL::Device* device, void* view) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsClassic();
    
    id mtlDevice = (__bridge id<MTLDevice>)device;
    MTKView* mtkView = (__bridge MTKView*)view;
    
    ImGui_ImplMetal_Init(mtlDevice);
    ImGui_ImplOSX_Init(mtkView);
    //[NSApp activateIgnoringOtherApps:YES];
}

void start_imgui_frame(void* view) {
    MTKView* mtkView = (__bridge MTKView*)view;
    
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize.x = mtkView.bounds.size.width;
    io.DisplaySize.y = mtkView.bounds.size.height;
    
    CGFloat framebufferScale = mtkView.window.screen.backingScaleFactor ?: NSScreen.mainScreen.backingScaleFactor;
    io.DisplayFramebufferScale = ImVec2(framebufferScale, framebufferScale);
    
    MTLRenderPassDescriptor* renderPassDescriptor = mtkView.currentRenderPassDescriptor;
    ImGui_ImplMetal_NewFrame(renderPassDescriptor);
    ImGui_ImplOSX_NewFrame(mtkView);
    
    ImGui::NewFrame();
}

void end_imgui_frame(void* view, MTL::CommandBuffer* commandBuffer) {
    ImGui::Render();
    
    id mtlCommandBuffer = (__bridge id<MTLCommandBuffer>)commandBuffer;
    MTKView* mtkView = (__bridge MTKView*)view;
    
    id <MTLRenderCommandEncoder> renderEncoder = [mtlCommandBuffer renderCommandEncoderWithDescriptor:mtkView.currentRenderPassDescriptor];
    [renderEncoder pushDebugGroup:@"Dear ImGui rendering"];
    ImDrawData* drawData = ImGui::GetDrawData();
    ImGui_ImplMetal_RenderDrawData(drawData, mtlCommandBuffer, renderEncoder);
    [renderEncoder popDebugGroup];
    [renderEncoder endEncoding];
}

} // namespace imgui_helper.
