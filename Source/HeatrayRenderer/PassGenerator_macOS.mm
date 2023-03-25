//
//  PassGenerator.cpp
//  Heatray5 macOS
//
//  Created by Cody White on 3/18/23.
//

#include "PassGenerator_macOS.hpp"

#include <Metal/Metal.h>
#include <MetalPerformanceShaders/MetalPerformanceShaders.h>

static constexpr size_t SIZEOF_RAY = 48;
static constexpr size_t INTERSECTION_SIZE = sizeof(MPSIntersectionDistancePrimitiveIndexCoordinates);
static constexpr size_t MAX_FRAMES_IN_FLIGHT = 3;

struct {
    MPSRayIntersector* rayIntersector = nullptr;
    MPSTriangleAccelerationStructure* accelerationStructure = nullptr;
    
    id <MTLLibrary> library;
    
    id <MTLBuffer> rayBuffer;
    id <MTLBuffer> shadowRayBuffer;
    id <MTLBuffer> intersectionBuffer;
    
    id <MTLTexture> accumulationTexture;
} raytracer;

PassGenerator::~PassGenerator() {
}

void PassGenerator::init(MTL::Device* device, const uint32_t renderWidth, const uint32_t renderHeight) {
    id mtlDevice = (__bridge id<MTLDevice>)device;
    
    // Setup core Metal objects.
    {
        raytracer.library = [mtlDevice newDefaultLibrary];
    }
    
    raytracer.rayIntersector = [[MPSRayIntersector alloc] initWithDevice:mtlDevice];
    raytracer.rayIntersector.rayDataType = MPSRayDataTypeOriginMaskDirectionMaxDistance;
    raytracer.rayIntersector.rayStride = SIZEOF_RAY;
    raytracer.rayIntersector.rayMaskOptions = MPSRayMaskOptionPrimitive;
    
    raytracer.accelerationStructure = [[MPSTriangleAccelerationStructure alloc] initWithDevice:mtlDevice];
    
    // Initialize the buffers for rendering that are based on screen size.
    resize(device, renderWidth, renderHeight);
}

void PassGenerator::destroy() {
    
}

void PassGenerator::resize(MTL::Device* device, const uint32_t newWidth, const uint32_t newHeight) {
    id mtlDevice = (__bridge id<MTLDevice>)device;
    
    size_t rayCount = newWidth * newHeight;
    raytracer.rayBuffer = [mtlDevice newBufferWithLength:SIZEOF_RAY * rayCount options:MTLResourceStorageModePrivate];
    raytracer.shadowRayBuffer = [mtlDevice newBufferWithLength:SIZEOF_RAY * rayCount options:MTLResourceStorageModePrivate];
    raytracer.intersectionBuffer = [mtlDevice newBufferWithLength:INTERSECTION_SIZE * rayCount options:MTLResourceStorageModePrivate];
    
    // Allocate a render target that the pathtracer will write to.
    {
        MTLTextureDescriptor *renderTargetDescriptor = [[MTLTextureDescriptor alloc] init];
        renderTargetDescriptor.pixelFormat = MTLPixelFormatRGBA32Float;
        renderTargetDescriptor.textureType = MTLTextureType2D;
        renderTargetDescriptor.width = newWidth;
        renderTargetDescriptor.height = newHeight;
        renderTargetDescriptor.storageMode = MTLStorageModePrivate;
        renderTargetDescriptor.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
        
        raytracer.accumulationTexture = [mtlDevice newTextureWithDescriptor:renderTargetDescriptor];
    }
}
