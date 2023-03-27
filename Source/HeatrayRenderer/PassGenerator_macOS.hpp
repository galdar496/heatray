//
//  PassGenerator.hpp
//  Heatray5 macOS
//
//  Created by Cody White on 3/18/23.
//

#pragma once

#include <Metal/MTLCommandBuffer.hpp>
#include <Metal/MTLDevice.hpp>
#include <Metal/MTLLibrary.hpp>

#include <cstdint>

class PassGenerator {
public:
    PassGenerator() = default;
    ~PassGenerator();
    
    //-------------------------------------------------------------------------
    // Initialize Heatray and get the pathtracer ready for use. Must be called
    // prior to doing any rendering.
    void init(MTL::Device* device, MTL::Library* shaderLibrary);
    
    //-------------------------------------------------------------------------
    // Deallocate any internal data and prepare for shutdown.
    void destroy();
    
    //-------------------------------------------------------------------------
    // Resize the pathtracer output. Parameters are in pixels.
    void resize(MTL::Device* device, const uint32_t newWidth, const uint32_t newHeight);
    
    //-------------------------------------------------------------------------
    // Encode the commands to render a pass (sample) of a frame.
    void encodePass(MTL::CommandBuffer* cmdBuffer);
    
private:
    uint32_t m_renderWidth = 0;
    uint32_t m_renderHeight = 0;
};
