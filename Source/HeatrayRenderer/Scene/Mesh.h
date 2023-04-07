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

#include <simd/simd.h>
#include <functional>
#include <memory>
#include <vector>

// Forward declarations.
namespace openrl {
class Buffer;
class Primitive;
class Program;
} // namespace openrl.
class MeshProvider;
class Material;

class Mesh
{
public:
    Mesh() = delete;

    Mesh(MeshProvider *meshProvider,
           std::vector<std::shared_ptr<Material>> &materials, 
           std::function<void(const std::shared_ptr<openrl::Program>)> & materialCreatedCallback,
           const simd::float4x4 &transform);
    ~Mesh() = default;

    // TODO:  Add accessors and stuff to mutate a RLMesh.  Right now it serves the
    // purpose of making sure all the RL resources get cleaned up and not much else!

    void destroy();

    bool valid() const { return m_indexBuffers.size() > 0; }

    const std::vector<std::shared_ptr<Material>>& materials() const { return m_materials;  }
    
    struct Submesh {
        std::shared_ptr<openrl::Primitive> primitive = nullptr;
        size_t elementCount = 0;
        size_t offset = 0;
        //RLenum mode = 0;
        std::shared_ptr<Material> material = nullptr;
        simd::float4x4 transform = matrix_identity_float4x4;
    };
    const std::vector<Submesh> &submeshes() const { return m_submeshes; }

private:
    std::vector<std::shared_ptr<MTL::Buffer*>> m_vertexBuffers;
    std::vector<std::shared_ptr<MTL::Buffer*>> m_indexBuffers;

    std::vector<Submesh> m_submeshes;

    std::vector<std::shared_ptr<Material>> m_materials;
};
