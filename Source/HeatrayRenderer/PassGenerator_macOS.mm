//
//  PassGenerator.cpp
//  Heatray5 macOS
//
//  Created by Cody White on 3/18/23.
//

#include "PassGenerator_macOS.hpp"

#include "Shaders/Ray.h"
#include "Shaders/RaytracerFrameConstants.h"
#include "Utility/Log.h"

#include <Metal/Metal.h>
#include <MetalPerformanceShaders/MetalPerformanceShaders.h>

static constexpr size_t SIZEOF_RAY = sizeof(Ray);
static constexpr size_t INTERSECTION_SIZE = sizeof(MPSIntersectionDistancePrimitiveIndexCoordinates);
static constexpr size_t MAX_FRAMES_IN_FLIGHT = 3;

struct AccumulationTextures {
    enum {
        READ = 0,
        WRITE = 1
    };
    
    void swap() {
        std::swap(rawTextures[READ], rawTextures[WRITE]);
    }
    
    id <MTLTexture> read() {
        return rawTextures[READ];
    }
    
    id <MTLTexture> write() {
        return rawTextures[WRITE];
    }
    
    // We use 2 textures to ping-pong back and forth from frame to frame.
    id <MTLTexture> rawTextures[2];
};

struct {
    MPSRayIntersector* rayIntersector = nullptr;
    MPSTriangleAccelerationStructure* accelerationStructure = nullptr;
    
    id <MTLLibrary> shaderLibrary;
    
    id <MTLBuffer> rayBuffer;
    id <MTLBuffer> shadowRayBuffer;
    id <MTLBuffer> intersectionBuffer;
    
    AccumulationTextures accumulationTextures;
    
    id <MTLComputePipelineState> raytracingPipelineState;
    
    // Per-frame constant buffers.
    struct {
        id <MTLBuffer> raytracerFrameConstants[MAX_FRAMES_IN_FLIGHT];
        
        // Advanced each time constant buffers are updated.
        size_t currentBufferIndex = 0;
    } perFrameConstants;
    
    // We use a semaphore to ensure that the CPU doesn't get ahead of the
    // MAX_FRAMES_IN_FLIGHT and garble GPU memory.
    dispatch_semaphore_t passSemaphore;
} raytracer;

PassGenerator::~PassGenerator() {
}

void createComputeShaderPipelines(id<MTLDevice> mtlDevice) {
    NSError* error = nullptr;
    
    MTLComputePipelineDescriptor *computeDescriptor = [[MTLComputePipelineDescriptor alloc] init];
    computeDescriptor.threadGroupSizeIsMultipleOfThreadExecutionWidth = YES;
    
    // Ray generation kernel.
    {
        computeDescriptor.computeFunction = [raytracer.shaderLibrary newFunctionWithName:@"generateFrame"];
        
        raytracer.raytracingPipelineState = [mtlDevice newComputePipelineStateWithDescriptor:computeDescriptor
                                                                                     options:0
                                                                                  reflection:nil
                                                                                       error:&error];
        if (!raytracer.raytracingPipelineState) {
            LOG_ERROR("Failed to create the raytracing compute pipeline: \"%s\"", [error.description cStringUsingEncoding:NSUTF8StringEncoding]);
        }
    }
}

void PassGenerator::init(MTL::Device* device, MTL::Library* shaderLibrary) {
    id mtlDevice = (__bridge id<MTLDevice>)device;
    raytracer.shaderLibrary = (__bridge id<MTLLibrary>)shaderLibrary;
    
    raytracer.rayIntersector = [[MPSRayIntersector alloc] initWithDevice:mtlDevice];
    raytracer.rayIntersector.rayDataType = MPSRayDataTypeOriginMaskDirectionMaxDistance;
    raytracer.rayIntersector.rayStride = SIZEOF_RAY;
    raytracer.rayIntersector.rayMaskOptions = MPSRayMaskOptionPrimitive;
    
    raytracer.accelerationStructure = [[MPSTriangleAccelerationStructure alloc] initWithDevice:mtlDevice];
    
    // Setup our raytracing shaders.
    createComputeShaderPipelines(mtlDevice);
    
    // Initialize all per-frame buffers.
    {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            raytracer.perFrameConstants.raytracerFrameConstants[i] = [mtlDevice newBufferWithLength:sizeof(RaytracerFrameConstants) options:MTLResourceStorageModeShared];
        }
    }
    
    raytracer.passSemaphore = dispatch_semaphore_create(MAX_FRAMES_IN_FLIGHT);
}

void PassGenerator::destroy() {
    
}

void PassGenerator::resize(MTL::Device* device, const uint32_t newWidth, const uint32_t newHeight) {
    
    // TODO: DELETE THIS STUFF AND RESIZE IT AS NECESSARY!!
    
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
        
        raytracer.accumulationTextures.rawTextures[AccumulationTextures::READ] = [mtlDevice newTextureWithDescriptor:renderTargetDescriptor];
        raytracer.accumulationTextures.rawTextures[AccumulationTextures::WRITE] = [mtlDevice newTextureWithDescriptor:renderTargetDescriptor];
    }
    
    m_renderWidth = newWidth;
    m_renderHeight = newHeight;
}

MTL::Texture* PassGenerator::encodePass(MTL::CommandBuffer* cmdBuffer, const RenderOptions& newRenderOptions) {
    // Use an 8x8 grid for threadgroup scheduling and round the current render width / height up to a multiple
    // of 8x8. Each shader will need to determine if the current thread is within the valid range before proceeding.
    MTLSize threadsPerThreadgroup = MTLSizeMake(8, 8, 1);
    MTLSize threadgroups = MTLSizeMake((m_renderWidth  + threadsPerThreadgroup.width  - 1) / threadsPerThreadgroup.width,
                                       (m_renderHeight + threadsPerThreadgroup.height - 1) / threadsPerThreadgroup.height,
                                       1);
    id mtlCmdBuffer = (__bridge id<MTLCommandBuffer>)cmdBuffer;
    
    id <MTLComputeCommandEncoder> computeEncoder = [mtlCmdBuffer computeCommandEncoder];
    
    // Advance to the next index in the ring buffers.
    raytracer.perFrameConstants.currentBufferIndex = (raytracer.perFrameConstants.currentBufferIndex + 1) % MAX_FRAMES_IN_FLIGHT;
    size_t currentBufferIndex = raytracer.perFrameConstants.currentBufferIndex;
    
    if (newRenderOptions.resetInternalState) {
        resetRenderingState(newRenderOptions);
    }
    
    // Before we start updating constants in our ring buffers, ensure that we're not too far ahead of the GPU.
    dispatch_semaphore_wait(raytracer.passSemaphore, DISPATCH_TIME_FOREVER);
    
    // Tell the command buffer to signal our semaphore once it's been completed so that we can resuse
    // ring buffer memory for the next pass.
    [mtlCmdBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
        dispatch_semaphore_signal(raytracer.passSemaphore);
    }];
    
    RaytracerFrameConstants* raytracerFrameConstants = static_cast<RaytracerFrameConstants*>(raytracer.perFrameConstants.raytracerFrameConstants[currentBufferIndex].contents);
    
    // Update the camera constants for this frame.
    {
        constexpr vector_float2 sensorDimensions{36.0f, 24.0f}; // Dimensions of 35mm film.
        // https://en.wikipedia.org/wiki/Angle_of_view#Calculating_a_camera's_angle_of_view
        const float fovY = 2.0f * std::atan2(sensorDimensions.y, 2.0f * m_renderOptions.camera.focalLength);
        raytracerFrameConstants->camera.fovTan = std::tanf(fovY * 0.5f);
        
        raytracerFrameConstants->camera.aspectRatio = m_renderOptions.camera.aspectRatio;
        raytracerFrameConstants->camera.viewMatrix = m_renderOptions.camera.viewMatrix;
        raytracerFrameConstants->camera.renderWidth = m_renderWidth;
        raytracerFrameConstants->camera.renderHeight = m_renderHeight;
    }
    
    // Update the extra frame data for this pass.
    {
        raytracerFrameConstants->sampleIndex = m_currentSampleIndex;
        raytracerFrameConstants->sampleIndex = m_shouldClear;
        m_shouldClear = false;
    }
    
    // Ray generation from the camera.
    {
        [computeEncoder pushDebugGroup:@"Raytracing"];
        [computeEncoder setBuffer:raytracer.perFrameConstants.raytracerFrameConstants[currentBufferIndex] offset:0 atIndex:0];
        [computeEncoder setTexture:raytracer.accumulationTextures.read() atIndex:0];
        [computeEncoder setTexture:raytracer.accumulationTextures.write() atIndex:1];
        [computeEncoder setComputePipelineState:raytracer.raytracingPipelineState];
        [computeEncoder dispatchThreadgroups:threadgroups threadsPerThreadgroup:threadsPerThreadgroup];
        [computeEncoder popDebugGroup];
        [computeEncoder endEncoding];
    }
    
    // Pass has been encoded, increase our sample index for the next pass.
    ++m_currentSampleIndex;
    
    // Swap the accumulation textures for the next frame.
    raytracer.accumulationTextures.swap();
    
    return (__bridge MTL::Texture*)raytracer.accumulationTextures.read();
}

void PassGenerator::resetRenderingState(const RenderOptions& newOptions) {
    m_currentSampleIndex = 0;
    m_shouldClear = true;
    
    // Copy all new settings.
    m_renderOptions = newOptions;
    
    m_renderOptions.resetInternalState = false;
}
