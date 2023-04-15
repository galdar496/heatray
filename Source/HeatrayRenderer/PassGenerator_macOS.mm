//
//  PassGenerator.cpp
//  Heatray5 macOS
//
//  Created by Cody White on 3/18/23.
//

#include "PassGenerator_macOS.hpp"

#include "Scene/Scene.h"
#include "Shaders/FrameConstants.h"
#include "Shaders/Ray.h"
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
    
    id <MTLComputePipelineState> cameraRayGeneratorPipelineState;
    id <MTLComputePipelineState> rayShaderPipelineState;
    
    // Per-frame constant buffers.
    struct {
        id <MTLBuffer> cameraRayGenerator[MAX_FRAMES_IN_FLIGHT];
        id <MTLBuffer> frameGlobals[MAX_FRAMES_IN_FLIGHT];
        
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
    
    // Camera ray generation kernel.
    {
        computeDescriptor.computeFunction = [raytracer.shaderLibrary newFunctionWithName:@"generateCameraRays"];
        
        raytracer.cameraRayGeneratorPipelineState = [mtlDevice newComputePipelineStateWithDescriptor:computeDescriptor
                                                                                             options:0
                                                                                          reflection:nil
                                                                                               error:&error];
        if (!raytracer.cameraRayGeneratorPipelineState) {
            LOG_ERROR("Failed to create the camera ray generator compute pipeline: \"%s\"", [error.description cStringUsingEncoding:NSUTF8StringEncoding]);
        }
        
        LOG_INFO("Created camera ray generation compute kernel");
    }
    
    // Ray shading kernel.
    {
        computeDescriptor.computeFunction = [raytracer.shaderLibrary newFunctionWithName:@"shadeRays"];
        
        raytracer.rayShaderPipelineState = [mtlDevice newComputePipelineStateWithDescriptor:computeDescriptor
                                                                                    options:0
                                                                                  reflection:nil
                                                                                       error:&error];
        if (!raytracer.rayShaderPipelineState) {
            LOG_ERROR("Failed to create the ray shading compute pipeline: \"%s\"", [error.description cStringUsingEncoding:NSUTF8StringEncoding]);
        }
        
        LOG_INFO("Created ray shading compute kernel");
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
            raytracer.perFrameConstants.cameraRayGenerator[i] = [mtlDevice newBufferWithLength:sizeof(CameraRayGeneratorConstants) options:MTLResourceStorageModeShared];
            raytracer.perFrameConstants.frameGlobals[i] = [mtlDevice newBufferWithLength:sizeof(FrameGlobals) options:MTLResourceStorageModeShared];
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
        
        if (m_statUpdateCallback) {
            // Determine how long this command buffer took to run.
            CFTimeInterval startTime = buffer.GPUStartTime;
            CFTimeInterval endTime = buffer.GPUEndTime;
            CFTimeInterval duration = endTime - startTime;
            
            RenderStats stats;
            stats.passGPUTimeSeconds = float(duration); // CFTimeInterval is in seconds.
            
            m_statUpdateCallback(stats);
        }
    }];
    
    // Update the globals for this frame.
    {
        FrameGlobals* globalConstants = static_cast<FrameGlobals*>(raytracer.perFrameConstants.frameGlobals[currentBufferIndex].contents);
        
        globalConstants->frameWidth = m_renderWidth;
        globalConstants->frameHeight = m_renderHeight;
        globalConstants->sampleIndex = m_currentSampleIndex;
    }
    
    // Update the camera constants for this frame.
    {
        CameraRayGeneratorConstants* cameraRayGeneratorConstants = static_cast<CameraRayGeneratorConstants*>(raytracer.perFrameConstants.cameraRayGenerator[currentBufferIndex].contents);
        
        constexpr vector_float2 sensorDimensions{36.0f, 24.0f}; // Dimensions of 35mm film.
        // https://en.wikipedia.org/wiki/Angle_of_view#Calculating_a_camera's_angle_of_view
        const float fovY = 2.0f * std::atan2(sensorDimensions.y, 2.0f * m_renderOptions.camera.focalLength);
        cameraRayGeneratorConstants->camera.fovTan = std::tanf(fovY * 0.5f);
        
        cameraRayGeneratorConstants->camera.aspectRatio = m_renderOptions.camera.aspectRatio;
        cameraRayGeneratorConstants->camera.viewMatrix = m_renderOptions.camera.viewMatrix;
        cameraRayGeneratorConstants->shouldClear = m_shouldClear;
        m_shouldClear = false;
    }
    
    // Ray generation from the camera.
    {
        id <MTLComputeCommandEncoder> computeEncoder = [mtlCmdBuffer computeCommandEncoder];
        [computeEncoder pushDebugGroup:@"Camera Ray Generation"];
        [computeEncoder setBuffer:raytracer.perFrameConstants.frameGlobals[currentBufferIndex] offset:0 atIndex:0];
        [computeEncoder setBuffer:raytracer.perFrameConstants.cameraRayGenerator[currentBufferIndex] offset:0 atIndex:1];
        [computeEncoder setBuffer:raytracer.rayBuffer offset:0 atIndex:2];
        [computeEncoder setTexture:raytracer.accumulationTextures.read() atIndex:0];
        [computeEncoder setComputePipelineState:raytracer.cameraRayGeneratorPipelineState];
        [computeEncoder dispatchThreadgroups:threadgroups threadsPerThreadgroup:threadsPerThreadgroup];
        [computeEncoder popDebugGroup];
        [computeEncoder endEncoding];
    }
    
    // Intersect the newly generated rays with the scene.
//    {
//        raytracer.rayIntersector.intersectionDataType = MPSIntersectionDataTypeDistancePrimitiveIndexCoordinates;
//
//        [raytracer.rayIntersector encodeIntersectionToCommandBuffer:mtlCmdBuffer
//                                                   intersectionType:MPSIntersectionTypeNearest
//                                                          rayBuffer:raytracer.rayBuffer
//                                                    rayBufferOffset:0
//                                                 intersectionBuffer:raytracer.intersectionBuffer
//                                           intersectionBufferOffset:0
//                                                           rayCount:m_renderWidth * m_renderHeight
//                                              accelerationStructure:raytracer.accelerationStructure];
//    }
//
//    // Shade the intersections.
//    {
//        id <MTLComputeCommandEncoder> computeEncoder = [mtlCmdBuffer computeCommandEncoder];
//    }
    
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

void PassGenerator::setScene(std::shared_ptr<Scene> scene) {
    // Read from the scene object and put its geometry into
    // an acceleration structure for intersection testing.
    assert(scene->meshes().size() == 1);
    for (const Mesh& mesh : scene->meshes()) {
        assert(mesh.submeshes().size() == 1);
        for (const Submesh& submesh : mesh.submeshes()) {
            raytracer.accelerationStructure.vertexBuffer = (__bridge id<MTLBuffer>)(mesh.vertexBuffer(submesh.vertexAttributes[util::to_underlying(VertexAttributeUsage::Position)].buffer));
            raytracer.accelerationStructure.vertexBufferOffset = submesh.vertexAttributes[util::to_underlying(VertexAttributeUsage::Position)].offset;
            raytracer.accelerationStructure.indexBuffer = (__bridge id<MTLBuffer>)mesh.indexBuffer(submesh.indexBuffer);
            raytracer.accelerationStructure.indexBufferOffset = submesh.indexOffset;
            
            // TODO: ADD OTHER VERTEX ATTRIBUTES HERE!!
        }
    }
    
    [raytracer.accelerationStructure rebuild];
}
