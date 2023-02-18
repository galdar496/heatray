#pragma once

#include "MeshProvider.h"

#include <HeatrayRenderer/Materials/Material.h>
#include <Utility/AABB.h>

#include "assimp/DefaultLogger.hpp"
#include "assimp/Importer.hpp"
#include "assimp/Logger.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "glm/glm/glm.hpp"

#include <memory>
#include <string>
#include <vector>

// Forward includes.
class Lighting;

class AssimpMeshProvider : public MeshProvider
{
public:
    explicit AssimpMeshProvider(const std::string &filename, bool convertToMeters, std::shared_ptr<Lighting> lighting);
    virtual ~AssimpMeshProvider() = default;

    size_t GetVertexBufferCount() override
    {
        return m_vertexBuffers.size();
    }

    size_t GetVertexBufferSize(size_t bufferIndex) override
    {
        return m_vertexBuffers[bufferIndex].size() * sizeof(float);
    }

    void FillVertexBuffer(size_t bufferIndex, uint8_t *buffer) override
    {
        memcpy(buffer, m_vertexBuffers[bufferIndex].data(), GetVertexBufferSize(bufferIndex));
    }

    size_t GetIndexBufferCount() override
    {
        return m_indexBuffers.size();
    }

    size_t GetIndexBufferSize(size_t bufferIndex) override
    {
        return m_indexBuffers[bufferIndex].size() * sizeof(int);
    }

    void FillIndexBuffer(size_t bufferIndex, uint8_t *buffer) override
    {
        memcpy(buffer, m_indexBuffers[bufferIndex].data(), GetIndexBufferSize(bufferIndex));
    }

    size_t GetSubmeshCount() override
    {
        return m_submeshes.size();
    }

    Submesh GetSubmesh(size_t submeshIndex) override
    {
        return m_submeshes[submeshIndex];
    }

    std::vector<std::shared_ptr<Material>> & GetMaterials()
    {
        return m_materials;
    }

    const util::AABB& sceneAABB() const { return m_sceneAABB; }

private:
    void LoadScene(std::string const & filename, std::shared_ptr<Lighting> lighting);
    void ProcessMesh(aiMesh const * mesh);
    void ProcessGlassMaterial(aiMaterial const* material);
    void ProcessMaterial(aiMaterial const * material);
    void ProcessLight(aiLight const * light, std::shared_ptr<Lighting> lighting, const aiScene* scene);
    void ProcessNode(aiScene const * scene, const aiNode * node, const aiMatrix4x4 & parentTransform, int level);

    std::string m_filename;

    std::vector<std::vector<float>> m_vertexBuffers;

    std::vector<std::vector<int>> m_indexBuffers;

    std::vector<Submesh> m_submeshes;

    std::vector<std::shared_ptr<Material>> m_materials;

    bool m_convertToMeters = false;

    util::AABB m_sceneAABB;
};
