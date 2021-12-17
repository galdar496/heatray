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
#include <string>

enum class DrawMode {
    Triangles,
    TriangleStrip,
};

enum VertexAttributeUsage {
    VertexAttributeUsage_Position,
    VertexAttributeUsage_Normal,
    VertexAttributeUsage_TexCoord,
    VertexAttributeUsage_Tangents,
    VertexAttributeUsage_Bitangents,
    VertexAttributeUsage_Colors,

    VertexAttributeUsageCount,
};

struct VertexAttribute {
    VertexAttributeUsage usage = VertexAttributeUsage_Position;
    int buffer = -1;
    int componentCount = 0;
    int size = 0;
    size_t offset = 0;
    int stride = 0;
};

class MeshProvider
{
public:
    struct Submesh {
        int vertexAttributeCount = 0;
        VertexAttribute vertexAttributes[VertexAttributeUsageCount];
        size_t indexBuffer = 0;
        size_t indexOffset = 0;
        size_t elementCount = 0;
        DrawMode drawMode = DrawMode::Triangles;
        int materialIndex = -1;
        glm::mat4 localTransform = glm::mat4(1.0f);
        std::string name;
    };

    explicit MeshProvider(const std::string &name) : m_name(name) {}
    virtual ~MeshProvider() {}
    virtual size_t GetVertexBufferCount() = 0;
    virtual size_t GetVertexBufferSize(size_t bufferIndex) = 0;
    virtual void FillVertexBuffer(size_t bufferIndex, uint8_t *buffer) = 0;

    virtual size_t GetIndexBufferCount() = 0;
    virtual size_t GetIndexBufferSize(size_t bufferIndex) = 0;
    virtual void FillIndexBuffer(size_t bufferIndex, uint8_t *buffer) = 0;

    virtual size_t GetSubmeshCount() = 0;
    virtual Submesh GetSubmesh(size_t submeshIndex) = 0;

    const std::string &name() { return m_name; }
protected:
    const std::string m_name;
};
