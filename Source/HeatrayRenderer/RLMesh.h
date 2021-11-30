//
//  RLMesh.h
//  Heatray
//
//  RLMesh is a helper class meant to encapsulate the geometry and
//  material references for a single mesh/model.
//
//

#pragma once

#include "Materials/Material.h"
#include "Scene/MeshProvider.h"

#include "RLWrapper/Buffer.h"
#include "RLWrapper/Primitive.h"
#include "RLWrapper/Program.h"

#include <glm/glm/mat4x4.hpp>

#include <functional>
#include <memory>
#include <vector>

class RLMesh
{
public:
    RLMesh() = default;

    using SetupSystemBindingsCallback = std::function<void(const std::shared_ptr<openrl::Program>)>;
    RLMesh(MeshProvider * meshProvider, std::vector<std::shared_ptr<Material>> materials, SetupSystemBindingsCallback callback, glm::mat4 transform);
    ~RLMesh() = default;

    // TODO:  Add accessors and stuff to mutate a RLMesh.  Right now it serves the
    // purpose of making sure all the RL resources get cleaned up and not much else!

    void destroy();

    bool valid() const { return m_indexBuffers.size() > 0; }

    const std::vector<std::shared_ptr<Material>>& materials() const { return m_materials;  }

private:
    std::vector<std::shared_ptr<openrl::Buffer>> m_vertexBuffers;
    std::vector<std::shared_ptr<openrl::Buffer>> m_indexBuffers;

    struct Submesh {
        std::shared_ptr<openrl::Primitive> primitive = nullptr;
        size_t elementCount = 0;
        size_t offset = 0;
        RLenum mode = 0;
        std::shared_ptr<Material> material = nullptr;
    };

    std::vector<Submesh> m_submeshes;

    std::vector<std::shared_ptr<Material>> m_materials;
};
