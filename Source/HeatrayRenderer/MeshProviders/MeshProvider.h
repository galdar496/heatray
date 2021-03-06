//
//  MeshProvider.h
//  Heatray
//
//  Interface that specifies a standard way to provide mesh data, intended
//  to make it easy to implement a variety of conforming providers that are
//  both driven by assets or computed at runtime.
//
//  Mesh data here means all vertex attribute data, vertex indices, and
//  submeshes.  Here, a Submesh is the vertex format, index to index buffer
//  in the set of index buffers, and draw info (type, count, offset).
//  Vertex format is usage-based, and is a list of vertex attributes with
//  buffer index, type, offset, stride, and size.
//
//

#pragma once

#include <glm/glm/mat4x4.hpp>

#include <stddef.h>
#include <stdint.h>

enum class DrawMode
{
    Triangles,
    TriangleStrip,
};

enum VertexAttributeUsage
{
    VertexAttributeUsage_Position,
    VertexAttributeUsage_Normal,
    VertexAttributeUsage_TexCoord,
    VertexAttributeUsage_TangentSpace,

    VertexAttributeUsageCount,
};

struct VertexAttribute
{
    VertexAttributeUsage usage;
    int buffer;
    int componentCount;
    int size;
    size_t offset;
    int stride;
};

class MeshProvider
{
public:
    struct Submesh
    {
        int vertexAttributeCount = 0;
        VertexAttribute vertexAttributes[4];
        size_t indexBuffer;
        size_t indexOffset;
        size_t elementCount;
        DrawMode drawMode;
        int materialIndex = -1;
        glm::mat4 localTransform;
    };

    virtual ~MeshProvider() {}
    virtual size_t GetVertexBufferCount() = 0;
    virtual size_t GetVertexBufferSize(size_t bufferIndex) = 0;
    virtual void FillVertexBuffer(size_t bufferIndex, uint8_t *buffer) = 0;

    virtual size_t GetIndexBufferCount() = 0;
    virtual size_t GetIndexBufferSize(size_t bufferIndex) = 0;
    virtual void FillIndexBuffer(size_t bufferIndex, uint8_t *buffer) = 0;

    virtual size_t GetSubmeshCount() = 0;
    virtual Submesh GetSubmesh(size_t submeshIndex) = 0;
};
