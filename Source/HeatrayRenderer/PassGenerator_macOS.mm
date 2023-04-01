//
//  PassGenerator.cpp
//  Heatray5 macOS
//
//  Created by Cody White on 3/18/23.
//

#include "PassGenerator_macOS.hpp"

#include "Shaders/GlobalData.h"
#include "Shaders/Ray.h"
#include "Shaders/RayGenerationConstants.h"
#include "Utility/Log.h"

#include <Metal/Metal.h>
#include <MetalPerformanceShaders/MetalPerformanceShaders.h>

static constexpr size_t SIZEOF_RAY = sizeof(Ray);
static constexpr size_t INTERSECTION_SIZE = sizeof(MPSIntersectionDistancePrimitiveIndexCoordinates);
static constexpr size_t MAX_FRAMES_IN_FLIGHT = 3;

struct {
    MPSRayIntersector* rayIntersector = nullptr;
    MPSTriangleAccelerationStructure* accelerationStructure = nullptr;
    
    id <MTLLibrary> shaderLibrary;
    
    id <MTLBuffer> rayBuffer;
    id <MTLBuffer> shadowRayBuffer;
    id <MTLBuffer> intersectionBuffer;
    
    id <MTLTexture> accumulationTexture;
    
    id <MTLComputePipelineState> rayGenerationPipelineState;
    
    // Per-frame constant buffers.
    struct {
        id <MTLBuffer> globalData[MAX_FRAMES_IN_FLIGHT];
        id <MTLBuffer> rayGeneration[MAX_FRAMES_IN_FLIGHT];
        
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
        computeDescriptor.computeFunction = [raytracer.shaderLibrary newFunctionWithName:@"rayGeneration"];
        
        raytracer.rayGenerationPipelineState = [mtlDevice newComputePipelineStateWithDescriptor:computeDescriptor
                                                                                        options:0
                                                                                     reflection:nil
                                                                                          error:&error];
        if (!raytracer.rayGenerationPipelineState) {
            LOG_ERROR("Failed to create the 'rayGeneration' compute pipeline: \"%s\"", [error.description cStringUsingEncoding:NSUTF8StringEncoding]);
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
            raytracer.perFrameConstants.globalData[i] = [mtlDevice newBufferWithLength:sizeof(GlobalData) options:MTLResourceStorageModeShared];
            raytracer.perFrameConstants.rayGeneration[i] = [mtlDevice newBufferWithLength:sizeof(RayGenerationConstants) options:MTLResourceStorageModeShared];
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
        
        raytracer.accumulationTexture = [mtlDevice newTextureWithDescriptor:renderTargetDescriptor];
    }
    
    m_renderWidth = newWidth;
    m_renderHeight = newHeight;
}

void PassGenerator::encodePass(MTL::CommandBuffer* cmdBuffer, const RenderOptions& newRenderOptions) {
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
    
    // Update the global data for this pass.
    {
        GlobalData* globalData = static_cast<GlobalData*>(raytracer.perFrameConstants.globalData[currentBufferIndex].contents);
        globalData->sampleIndex = m_currentSampleIndex;
    }
    
    // Ray generation from the camera.
    {
        // Update the constants necessary for ray generation.
        {
            RayGenerationConstants* constants = static_cast<RayGenerationConstants*>(raytracer.perFrameConstants.rayGeneration[currentBufferIndex].contents);
            
            constexpr vector_float2 sensorDimensions{36.0f, 24.0f}; // Dimensions of 35mm film.
            // https://en.wikipedia.org/wiki/Angle_of_view#Calculating_a_camera's_angle_of_view
            const float fovY = 2.0f * std::atan2(sensorDimensions.y, 2.0f * m_renderOptions.camera.focalLength);
            constants->fovTan = std::tanf(fovY * 0.5f);
            
            constants->aspectRatio = m_renderOptions.camera.aspectRatio;
            constants->viewMatrix = m_renderOptions.camera.viewMatrix;
            constants->renderWidth = m_renderWidth;
            constants->renderHeight = m_renderHeight;
        }
        
        [computeEncoder pushDebugGroup:@"Camera ray generation"];
        [computeEncoder setBuffer:raytracer.perFrameConstants.rayGeneration[currentBufferIndex] offset:0 atIndex:0];
        [computeEncoder setBuffer:raytracer.rayBuffer                                           offset:0 atIndex:1];
        [computeEncoder setComputePipelineState:raytracer.rayGenerationPipelineState];
        [computeEncoder dispatchThreadgroups:threadgroups threadsPerThreadgroup:threadsPerThreadgroup];
        [computeEncoder popDebugGroup];
        [computeEncoder endEncoding];
    }
    
    // Pass has been encoded, increase our sample index for the next pass.
    ++m_currentSampleIndex;
}

void PassGenerator::resetRenderingState(const RenderOptions& newOptions) {
    m_currentSampleIndex = 0;
    
    // TODO: NEED TO DO SOMETHING TO CLEAR THE TEXTURE!!. PROBABLY A CUSTOM COMPUTE KERNEL.
    
    // Copy all new settings.
    m_renderOptions = newOptions;
    
    m_renderOptions.resetInternalState = false;
}
