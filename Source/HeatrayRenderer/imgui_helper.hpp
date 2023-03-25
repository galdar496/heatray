//
//  imgui_initializer.hpp
//  Heatray5 macOS
//
//  Created by Cody White on 3/23/23.
//

#pragma once

#include <imgui.h>
#include <Metal/MTLDevice.hpp>
#include <Metal/MTLCommandBuffer.hpp>

namespace imgui_helper {

void init_imgui(MTL::Device* device, void* view);

void start_imgui_frame(void* view);

void end_imgui_frame(void* view, MTL::CommandBuffer* commandBuffer);

} // namespace imgui_helper.
