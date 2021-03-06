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
#include "MeshProviders/MeshProvider.h"

#include "RLWrapper/Buffer.h"
#include "RLWrapper/Primitive.h"
#include "RLWrapper/Program.h"

#include <glm/glm/mat4x4.hpp>

#include <functional>
#include <vector>

class RLMesh
{
public:
    RLMesh() = default;

    using SetupSystemBindingsCallback = std::function<void(const openrl::Program&)>;
    RLMesh(MeshProvider * meshProvider, std::vector<Material *> materials, SetupSystemBindingsCallback callback, glm::mat4 transform);
    ~RLMesh() = default;

    // TODO:  Add accessors and stuff to mutate an RLMesh.  Right now it serves the
    // purpose of making sure all the RL resources get cleaned up and not much else!

    void destroy();

private:
    std::vector<openrl::Buffer> m_vertexBuffers;
    std::vector<openrl::Buffer> m_indexBuffers;

    struct Submesh
    {
        openrl::Primitive primitive;
        size_t elementCount;
        size_t offset;
        RLenum mode;
        Material* material = nullptr;
    };

    std::vector<Submesh> m_submeshes;

    std::vector<Material *> m_materials;
};
