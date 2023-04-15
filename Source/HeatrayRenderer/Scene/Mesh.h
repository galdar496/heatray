//
//  Mesh.h
//  Heatray
//
//  Mesh is a helper class meant to encapsulate the geometry and
//  material references for a single mesh/model.
//
//

#pragma once

#include "Submesh.h"
#include "VertexAttribute.h"

#include <Utility/Util.h>

#include <Metal/MTLArgumentEncoder.hpp>
#include <Metal/MTLBuffer.hpp>
#include <Metal/MTLDevice.hpp>

#include <array>
#include <simd/simd.h>
#include <functional>
#include <memory>
#include <vector>

// Forward declarations.
class MeshProvider;
class Material;

class Mesh {
public:
    Mesh() = delete;

    Mesh(MeshProvider* meshProvider,
           std::vector<std::shared_ptr<Material>>& materials,
           const simd::float4x4 &transform,
           MTL::Device* device);
    ~Mesh() { destroy(); }

    void destroy();

    bool valid() const { return m_submeshes.size() > 0; }

    const std::vector<std::shared_ptr<Material>>& materials() const { return m_materials;  }
    
    const std::vector<Submesh> &submeshes() const { return m_submeshes; }
    
    MTL::Buffer* vertexBuffer(size_t bufferIndex) const { return m_vertexBuffers[bufferIndex]; }
    MTL::Buffer* indexBuffer(size_t bufferIndex) const { return m_indexBuffers[bufferIndex]; }

private:
    // All vertex / index buffers are stored here. Each submesh
    // will reference a collection of these buffers as necessary.
    std::vector<MTL::Buffer*> m_vertexBuffers;
    std::vector<MTL::Buffer*> m_indexBuffers;
    
    std::vector<Submesh> m_submeshes;
    std::vector<std::shared_ptr<Material>> m_materials;
    
    MTL::ArgumentEncoder* resourceEncoder = nullptr;
};
