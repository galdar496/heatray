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
#include <Metal/MTLTexture.hpp>

#include <cstdint>
#include <simd/simd.h>

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
    // These options control the current state of the renderer are are used
    // when resetting the renderer.
    struct RenderOptions {
        // If true, the internal state of the pass generator is reset.
        bool resetInternalState = true;
        
        struct Camera {
            static constexpr size_t NUM_FSTOPS = 12;
            static constexpr float fstopOptions[NUM_FSTOPS] = {
                std::numeric_limits<float>::max(), 32.0f, 22.0f, 16.0f, 11.0f, 8.0f, 5.6f, 4.0f, 2.8f, 2.0f, 1.4f, 1.0f
            };

            float aspectRatio    = -1.0f;  // Width / height.
            float focusDistance  = 1.0f;   // In meters.
            float focalLength    = 50.0f;  // In millimeters.
            float apertureRadius = 0.0f;   // In meters (should not be set manually!).
            float fstop             = fstopOptions[1];
            matrix_float4x4 viewMatrix;

            //-------------------------------------------------------------------------
            // Should be called to update the internal aperture radius based off various
            // camera parameters.
            void setApertureRadius() {
                apertureRadius = (focalLength / fstop) / 1000.0f; // Value is in meters.
            }
        } camera;
    };
    
    //-------------------------------------------------------------------------
    // Encode the commands to render a pass (sample) of a frame. The returned
    // texture is the current state of the raytracer (including all previous
    // passes since the last time the generator was reset).
    MTL::Texture* encodePass(MTL::CommandBuffer* cmdBuffer, const RenderOptions& newRenderOptions);
    
private:
    void resetRenderingState(const RenderOptions& newOptions);
    
    uint32_t m_renderWidth = 0;
    uint32_t m_renderHeight = 0;
    uint32_t m_currentSampleIndex = 0;
    bool m_shouldClear = true;
    
    RenderOptions m_renderOptions;
};
