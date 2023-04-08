//
//  Mesh.h
//  Heatray
//
//  Mesh is a helper class meant to encapsulate the geometry and
//  material references for a single mesh/model.
//
//

#pragma once

#include <Metal/MTLBuffer.hpp>
#include <Metal/MTLDevice.hpp>

#include <simd/simd.h>
#include <functional>
#include <memory>
#include <vector>

// Forward declarations.
class MeshProvider;
class Material;

class Mesh
{
public:
    Mesh() = delete;

    Mesh(MeshProvider* meshProvider,
           std::vector<std::shared_ptr<Material>>& materials,
           const simd::float4x4 &transform,
           MTL::Device* device);
    ~Mesh() = default;

    void destroy();

    bool valid() const { return m_indexBuffers.size() > 0; }

    const std::vector<std::shared_ptr<Material>>& materials() const { return m_materials;  }
    
    struct Submesh {
        size_t elementCount = 0;
        size_t offset = 0;
        //RLenum mode = 0;
        std::shared_ptr<Material> material = nullptr;
        simd::float4x4 transform = matrix_identity_float4x4;
    };
    const std::vector<Submesh> &submeshes() const { return m_submeshes; }

private:
    std::vector<MTL::Buffer*> m_vertexBuffers;
    std::vector<MTL::Buffer*> m_indexBuffers;

    std::vector<Submesh> m_submeshes;

    std::vector<std::shared_ptr<Material>> m_materials;
};
