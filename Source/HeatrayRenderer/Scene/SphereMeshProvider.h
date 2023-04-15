//
//  SphereMeshProvider.h
//  Heatray
//
//  MeshProvider that computes a lat/lon tesselated sphere mesh.  The
//  vertex data is planar, and position, normal and tex coord are
//  computed (no tangent spaces yet).
//
//

#pragma once

#include "MeshProvider.h"
#include "VertexAttribute.h"

#include <Utility/Math.h>

#include <simd/simd.h>

simd::float3 CartesianFromSpherical(simd::float3 const & spherical)
{
    return simd::make_float3(spherical.x * std::cos(spherical.y) * std::sin(spherical.z),
                             spherical.x * std::cos(spherical.z),
                             spherical.x * std::sin(spherical.y) * std::sin(-spherical.z));
}

class SphereMeshProvider : public MeshProvider
{
public:
    explicit SphereMeshProvider(size_t uSlices, size_t vSlices, float radius, const std::string_view name) : MeshProvider(name), uSlices(uSlices), vSlices(vSlices), radius(radius)
    {
        vertexCount = (uSlices + 1) * (vSlices + 2);
    }
    virtual ~SphereMeshProvider() {}

    size_t GetVertexBufferCount() override
    {
        return 3;
    }

    size_t GetVertexBufferSize(size_t bufferIndex) override
    {
        size_t componentSizes[3] = {
            sizeof(simd::float3), // Position
            sizeof(simd::float3), // Normal
            sizeof(simd::float2)  // UV
        };

        return vertexCount * componentSizes[bufferIndex];
    }

    void FillVertexBuffer(size_t bufferIndex, uint8_t *buffer) override
    {
        constexpr size_t stepSize = sizeof(simd::float3) / sizeof(float);
        
        float *floatPtr = (float *)buffer;
        size_t vSteps = vSlices + 2;
        for (size_t ii = 0; ii < uSlices + 1; ++ii) {
            float u = (float)ii / (float)uSlices;
            // Add 2 to vSlices so we account for the poles
            for (size_t jj = 0; jj < vSteps; ++jj) {
                // Add 1 to vSlices for our range for the poles
                float v = (float)jj / (float)(vSlices + 1);

                if (bufferIndex == 0 || bufferIndex == 1) {
                    const simd::float3 spherical = simd::make_float3(radius, float(u * util::constants::TWO_PI), float(v * util::constants::PI));
                    simd::float3 p = CartesianFromSpherical(spherical);
                    if (bufferIndex == 1) {
                        p = simd::normalize(p);
                    }
                    floatPtr[0] = p.x;
                    floatPtr[1] = p.y;
                    floatPtr[2] = p.z;
                    floatPtr += stepSize;
                } else {
                    *floatPtr = u;
                    ++floatPtr;
                    *floatPtr = 1.f - v;
                    ++floatPtr;
                }
            }
        }
    }

    size_t GetIndexBufferCount() override
    {
        return 1;
    }

    size_t GetIndexBufferSize(size_t bufferIndex) override
    {
        size_t triangleCount = 2 * uSlices * vSlices;
        return 3 * triangleCount * sizeof(int);
    }

    void FillIndexBuffer(size_t bufferIndex, uint8_t *buffer) override
    {
        uint32_t *indexPtr = (uint32_t *)buffer;

        int vSteps = (int)vSlices + 2;
        for (int ii = 0; ii < (int)uSlices; ++ii) {
            for (int jj = 0; jj < vSteps - 1; ++jj) {
                // If jj is adjacent to first pole, make a triangles
                if (jj == 0) {
                    // first pole, make a triangle
                    *indexPtr = (ii + 0) * vSteps;
                    ++indexPtr;
                    *indexPtr = (ii + 0) * vSteps + 1;
                    ++indexPtr;
                    *indexPtr = (ii + 1) * vSteps + 1;
                    ++indexPtr;
                } else if (jj == vSteps - 2) {
                    // second pole, make a triangle
                    *indexPtr = (ii + 1) * vSteps + (jj + 0);
                    ++indexPtr;
                    *indexPtr = (ii + 0) * vSteps + (jj + 0);
                    ++indexPtr;
                    *indexPtr = (ii + 0) * vSteps + (jj + 1);
                    ++indexPtr;
                } else {
                    // Make two triangles for this quad of the v slice
                    *indexPtr = (ii + 0) * vSteps + (jj + 0);
                    ++indexPtr;
                    *indexPtr = (ii + 0) * vSteps + (jj + 1);
                    ++indexPtr;
                    *indexPtr = (ii + 1) * vSteps + (jj + 1);
                    ++indexPtr;

                    *indexPtr = (ii + 1) * vSteps + (jj + 1);
                    ++indexPtr;
                    *indexPtr = (ii + 1) * vSteps + (jj + 0);
                    ++indexPtr;
                    *indexPtr = (ii + 0) * vSteps + (jj + 0);
                    ++indexPtr;
                }
            }
        }
    }

    size_t GetSubmeshCount() override
    {
        return 1;
    }

    Submesh GetSubmesh(size_t submeshIndex) override
    {
        Submesh submesh;

        submesh.vertexAttributeCount = 3;
        submesh.vertexAttributes[0].usage = VertexAttributeUsage::Position;
        submesh.vertexAttributes[0].buffer = 0;
        submesh.vertexAttributes[0].componentCount = 3;
        submesh.vertexAttributes[0].size = sizeof(float);
        submesh.vertexAttributes[0].offset = 0;
        submesh.vertexAttributes[0].stride = sizeof(simd::float3);

        submesh.vertexAttributes[1].usage = VertexAttributeUsage::Normal;
        submesh.vertexAttributes[1].buffer = 1;
        submesh.vertexAttributes[1].componentCount = 3;
        submesh.vertexAttributes[1].size = sizeof(float);
        submesh.vertexAttributes[1].offset = 0;
        submesh.vertexAttributes[1].stride = sizeof(simd::float3);

        submesh.vertexAttributes[2].usage = VertexAttributeUsage::TexCoord;
        submesh.vertexAttributes[2].buffer = 2;
        submesh.vertexAttributes[2].componentCount = 2;
        submesh.vertexAttributes[2].size = sizeof(float);
        submesh.vertexAttributes[2].offset = 0;
        submesh.vertexAttributes[2].stride = sizeof(simd::float2);

        submesh.indexBuffer = 0;
        submesh.indexOffset = 0;
        size_t triangleCount = 2 * uSlices * vSlices;
        submesh.elementCount = 3 * triangleCount;

        submesh.drawMode = DrawMode::Triangles;
        submesh.localTransform = matrix_identity_float4x4;
        submesh.name = "Sphere";

        return submesh;
    }

private:
    size_t uSlices = 0;
    size_t vSlices = 0;
    float radius = 0.0f;
    size_t vertexCount = 0;
};
