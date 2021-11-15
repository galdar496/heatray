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

#include "glm/glm/glm.hpp"

glm::vec3 CartesianFromSpherical(glm::vec3 const & spherical)
{
    return glm::vec3(spherical.x * std::cos(spherical.y) * std::sin(spherical.z),
                     spherical.x * std::cos(spherical.z),
                     spherical.x * std::sin(spherical.y) * std::sin(-spherical.z));
}

class SphereMeshProvider : public MeshProvider
{
public:
    explicit SphereMeshProvider(int uSlices, int vSlices, float radius) : MeshProvider(), uSlices(uSlices), vSlices(vSlices), radius(radius)
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
        size_t componentCounts[3] = {3, 3, 2};

        return vertexCount * componentCounts[bufferIndex] * sizeof(float);
    }

    void FillVertexBuffer(size_t bufferIndex, uint8_t *buffer) override
    {
        float *floatPtr = (float *)buffer;
        int vSteps = vSlices + 2;
        for (int ii = 0; ii < uSlices + 1; ++ii) {
            float u = (float)ii / (float)uSlices;
            // Add 2 to vSlices so we account for the poles
            for (int jj = 0; jj < vSteps; ++jj) {
                // Add 1 to vSlices for our range for the poles
                float v = (float)jj / (float)(vSlices + 1);

                if (bufferIndex == 0 || bufferIndex == 1) {
                    const glm::vec3 spherical(radius, float(u * glm::two_pi<float>()), float(v * glm::pi<float>()));
                    glm::vec3 p = CartesianFromSpherical(spherical);
                    if (bufferIndex == 1) {
                        p = glm::normalize(p);
                    }
                    memcpy(floatPtr, &p.x, sizeof(glm::vec3));
                    floatPtr += 3;
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
        int *indexPtr = (int *)buffer;

        int vSteps = vSlices + 2;
        for (int ii = 0; ii < uSlices; ++ii) {
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
        submesh.vertexAttributes[0].usage = VertexAttributeUsage_Position;
        submesh.vertexAttributes[0].buffer = 0;
        submesh.vertexAttributes[0].componentCount = 3;
        submesh.vertexAttributes[0].size = sizeof(float);
        submesh.vertexAttributes[0].offset = 0;
        submesh.vertexAttributes[0].stride = 3 * sizeof(float);

        submesh.vertexAttributes[1].usage = VertexAttributeUsage_Normal;
        submesh.vertexAttributes[1].buffer = 1;
        submesh.vertexAttributes[1].componentCount = 3;
        submesh.vertexAttributes[1].size = sizeof(float);
        submesh.vertexAttributes[1].offset = 0;
        submesh.vertexAttributes[1].stride = 3 * sizeof(float);

        submesh.vertexAttributes[2].usage = VertexAttributeUsage_TexCoord;
        submesh.vertexAttributes[2].buffer = 2;
        submesh.vertexAttributes[2].componentCount = 2;
        submesh.vertexAttributes[2].size = sizeof(float);
        submesh.vertexAttributes[2].offset = 0;
        submesh.vertexAttributes[2].stride = 2 * sizeof(float);

        submesh.indexBuffer = 0;
        submesh.indexOffset = 0;
        size_t triangleCount = 2 * uSlices * vSlices;
        submesh.elementCount = 3 * triangleCount;

        submesh.drawMode = DrawMode::Triangles;
        submesh.localTransform = glm::mat4(1.0f); // identity.

        return submesh;
    }

private:
    int uSlices = 0;
    int vSlices = 0;
    float radius = 0.0f;
    size_t vertexCount = 0;
};
