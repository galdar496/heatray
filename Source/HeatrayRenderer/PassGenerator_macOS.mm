//
//  PassGenerator.cpp
//  Heatray5 macOS
//
//  Created by Cody White on 3/18/23.
//

#include "PassGenerator_macOS.hpp"

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
        id <MTLBuffer> rayGeneration[MAX_FRAMES_IN_FLIGHT];
        
        // Advanced each time constant buffers are updated.
        size_t currentBufferIndex = 0;
    } perFrameConstants;
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
            raytracer.perFrameConstants.rayGeneration[i] = [mtlDevice newBufferWithLength:sizeof(RayGenerationConstants) options:MTLResourceStorageModeShared];
        }
    }
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

void PassGenerator::encodePass(MTL::CommandBuffer* cmdBuffer) {
    // Use an 8x8 grid for threadgroup scheduling and round the current render width / height up to a multiple
    // of 8x8. Each shader will need to determine if the current thread is within the valid range before proceeding.
    MTLSize threadsPerThreadgroup = MTLSizeMake(8, 8, 1);
    MTLSize threadgroups = MTLSizeMake((m_renderWidth  + threadsPerThreadgroup.width  - 1) / threadsPerThreadgroup.width,
                                       (m_renderHeight + threadsPerThreadgroup.height - 1) / threadsPerThreadgroup.height,
                                       1);
    id mtlCmdBuffer = (__bridge id<MTLCommandBuffer>)cmdBuffer;
    
    id <MTLComputeCommandEncoder> computeEncoder = [mtlCmdBuffer computeCommandEncoder];
    
    // Ray generation from the camera.
    {
        [computeEncoder pushDebugGroup:@"Camera ray generation"];
        [computeEncoder setBuffer:raytracer.perFrameConstants.rayGeneration[0] offset:0 atIndex:0];
        [computeEncoder setBuffer:raytracer.rayBuffer                          offset:0 atIndex:1];
        [computeEncoder setComputePipelineState:raytracer.rayGenerationPipelineState];
        [computeEncoder dispatchThreadgroups:threadgroups threadsPerThreadgroup:threadsPerThreadgroup];
        [computeEncoder popDebugGroup];
        [computeEncoder endEncoding];
    }
}
