//
//  HeatrayRenderer.hpp
//  Heatray5 macOS
//
//  Created by Cody White on 3/18/23.
//

#pragma once

#include "PassGenerator_macOS.hpp"

#include <Metal/MTLBuffer.hpp>
#include <Metal/MTLCommandBuffer.hpp>
#include <Metal/MTLCommandQueue.hpp>
#include <Metal/MTLDevice.hpp>
#include <Metal/MTLLibrary.hpp>
#include <Metal/MTLRenderPipeline.hpp>
#include <MetalKit/MTKView.hpp>

#include <cstdint>

class HeatrayRenderer {
public:
    HeatrayRenderer() {
        
    }
    ~HeatrayRenderer() = default;
    
    // Fixed pixel width of the ImGui UI.
    static constexpr uint32_t UI_WINDOW_WIDTH = 500;
    
    //-------------------------------------------------------------------------
    // Initialize Heatray for use given a specific window width and height
    // (in pixels).
    bool init(MTL::Device* device, MTK::View* view, const uint32_t renderWidth, const uint32_t renderHeight);

    //-------------------------------------------------------------------------
    // Tear down Heatray and delete all resources.
    void destroy();
    
    //-------------------------------------------------------------------------
    // Resize the rendering window and any associated window-sized data.
    void resize(const uint32_t newWidth, const uint32_t newHeight);
    
    //-------------------------------------------------------------------------
    // Render a frame.
    void render(MTK::View* view);
    
    //-------------------------------------------------------------------------
    // Move the orbital camera. Useful for mouse-style controls.
    void adjustCamera(const float phiDelta, const float thetaDelta, const float distanceDelta);

    //-------------------------------------------------------------------------
    // Reset the renderer's internal state.
    void resetRenderer();
    
private:
    void setupDisplayShader(const MTK::View* view);
    void encodeDisplay(MTK::View* view, MTL::RenderCommandEncoder* encoder);
    bool encodeUI(MTK::View* view, MTL::CommandBuffer* cmdBuffer, MTL::RenderCommandEncoder* encoder);
    
    MTL::Device* m_device = nullptr;
    MTL::Library* m_shaderLibrary = nullptr;
    MTL::CommandQueue* m_commandQueue = nullptr;
    
    PassGenerator m_passGenerator; // Generator of pathtraced frames.
    
    struct WindowParams {
        uint32_t width = ~0;
        uint32_t height = ~0;
    };
    
    WindowParams m_windowParams; // Current size of the display window.
    WindowParams m_renderWindowParams; // Current size of the rendering window.
    
    struct {
        MTL::RenderPipelineState* pipelineState = nullptr;
        MTL::Buffer* vertexConstants = nullptr;
    } m_display;
    
    PassGenerator::RenderOptions m_renderOptions;
};
