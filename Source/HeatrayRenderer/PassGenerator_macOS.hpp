//
//  PassGenerator.hpp
//  Heatray5 macOS
//
//  Created by Cody White on 3/18/23.
//

#pragma once

#include <Metal/MTLDevice.hpp>

#include <cstdint>

class PassGenerator {
public:
    PassGenerator() = default;
    ~PassGenerator();
    
    //-------------------------------------------------------------------------
    // Initialize Heatray and get the pathtracer ready for use. Must be called
    // prior to doing any rendering. Parameters are in pixels.
    void init(MTL::Device* device, const uint32_t renderWidth, const uint32_t renderHeight);
    
    //-------------------------------------------------------------------------
    // Deallocate any internal data and prepare for shutdown.
    void destroy();
    
    //-------------------------------------------------------------------------
    // Resize the pathtracer output. Parameters are in pixels.
    void resize(MTL::Device* device, const uint32_t newWidth, const uint32_t newHeight);
    
private:
};
