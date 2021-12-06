//
//  SphereMeshProvider.h
//  Heatray
//
//  MeshProvider that computes a plane oriented along the XZ axis facing Y up.
//
//

#pragma once

#include "MeshProvider.h"

#include <glm/glm/glm.hpp>

#include <assert.h>

class PlaneMeshProvider : public MeshProvider
{
enum BufferTypes {
    kPositions = 0,
    kNormals,
    kUVs
};

public:
    explicit PlaneMeshProvider(size_t width, size_t length, const std::string &name) : MeshProvider(name), m_width(width), m_length(length)
    {
    }
    virtual ~PlaneMeshProvider() {}

    size_t GetVertexBufferCount() override { return 3; }
    size_t GetVertexBufferSize(size_t bufferIndex) override
    {
        // Positions/normals/UVs.
        constexpr size_t componentCounts[3] = { 3, 3, 2 };

        // Plane is made up of 4 vertices.
        return 4 * componentCounts[bufferIndex] * sizeof(float);
    }
    void FillVertexBuffer(size_t bufferIndex, uint8_t* buffer) override
    {
        assert(buffer);
        float* floatPtr = reinterpret_cast<float*>(buffer);
        switch (bufferIndex) {
            case kPositions: // Positions.
            {
                glm::vec3 scale(m_width * 0.5f, 0.0f, m_length * 0.5f);
                glm::vec3 vertices[4] = { glm::vec3(-1.0f, 0.0f,  1.0f) * scale,
                                          glm::vec3( 1.0f, 0.0f,  1.0f) * scale,
                                          glm::vec3( 1.0f, 0.0f, -1.0f) * scale,
                                          glm::vec3(-1.0f, 0.0f, -1.0f) * scale 
                                        };

                
                memcpy(floatPtr, &vertices[0].x, GetVertexBufferSize(0));
                break;
            }
            case kNormals: // Normals.
            {
                glm::vec3 normals[4] = { glm::vec3(0.0f, 1.0f, 0.0f),
                                         glm::vec3(0.0f, 1.0f, 0.0f),
                                         glm::vec3(0.0f, 1.0f, 0.0f),
                                         glm::vec3(0.0f, 1.0f, 0.0f)
                                       };
                memcpy(floatPtr, &normals[0].x, GetVertexBufferSize(1));
                break;
            }
            case kUVs: // UVs.
            {
                glm::vec2 uvs[4] = { glm::vec2(-1.0f, -1.0f),
                                     glm::vec2( 1.0f, -1.0f),
                                     glm::vec2( 1.0f,  1.0f),
                                     glm::vec2(-1.0f,  1.0f)
                                   };
                memcpy(floatPtr, &uvs[0].x, GetVertexBufferSize(2));
                break;
            }
            default:
                assert(0);
        }
    }

    size_t GetIndexBufferCount() override { return 1; }
    size_t GetIndexBufferSize(size_t bufferIndex) override
    {
        return 4 * sizeof(int);
    }
    void FillIndexBuffer(size_t bufferIndex, uint8_t* buffer) override
    {
        assert(buffer);
        // All buffers use the same layout.
        int indices[4] = { 0, 1, 3, 2 };
        int* intPtr = reinterpret_cast<int*>(buffer);
        memcpy(intPtr, indices, GetIndexBufferSize(bufferIndex));
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
        submesh.vertexAttributes[0].buffer = kPositions;
        submesh.vertexAttributes[0].componentCount = 3;
        submesh.vertexAttributes[0].size = sizeof(float);
        submesh.vertexAttributes[0].offset = 0;
        submesh.vertexAttributes[0].stride = 3 * sizeof(float);

        submesh.vertexAttributes[1].usage = VertexAttributeUsage_Normal;
        submesh.vertexAttributes[1].buffer = kNormals;
        submesh.vertexAttributes[1].componentCount = 3;
        submesh.vertexAttributes[1].size = sizeof(float);
        submesh.vertexAttributes[1].offset = 0;
        submesh.vertexAttributes[1].stride = 3 * sizeof(float);

        submesh.vertexAttributes[2].usage = VertexAttributeUsage_TexCoord;
        submesh.vertexAttributes[2].buffer = kUVs;
        submesh.vertexAttributes[2].componentCount = 2;
        submesh.vertexAttributes[2].size = sizeof(float);
        submesh.vertexAttributes[2].offset = 0;
        submesh.vertexAttributes[2].stride = 2 * sizeof(float);

        submesh.indexBuffer = 0;
        submesh.indexOffset = 0;
        size_t triangleCount = 2;
        submesh.elementCount = 4;

        submesh.drawMode = DrawMode::TriangleStrip;
        submesh.localTransform = glm::mat4(1.0f); // identity.

        return submesh;
    }

private:
    size_t m_width = 0;  // X
    size_t m_length = 0; // Z
};