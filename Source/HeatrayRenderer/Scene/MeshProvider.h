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

#include "Submesh.h"
#include "VertexAttribute.h"

#include "Utility/Util.h"

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <string_view>

class MeshProvider
{
public:
    explicit MeshProvider(const std::string_view name) : m_name(name) {}
    virtual ~MeshProvider() {}
    virtual size_t GetVertexBufferCount() = 0;
    virtual size_t GetVertexBufferSize(size_t bufferIndex) = 0;
    virtual void FillVertexBuffer(size_t bufferIndex, uint8_t *buffer) = 0;

    virtual size_t GetIndexBufferCount() = 0;
    virtual size_t GetIndexBufferSize(size_t bufferIndex) = 0;
    virtual void FillIndexBuffer(size_t bufferIndex, uint8_t *buffer) = 0;

    virtual size_t GetSubmeshCount() = 0;
    virtual Submesh GetSubmesh(size_t submeshIndex) = 0;

    const std::string_view name() { return m_name; }
protected:
    const std::string m_name;
};
