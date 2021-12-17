#define GLM_FORCE_SWIZZLE

#include "AssimpMeshProvider.h"

#include "Lighting.h"

#include "HeatrayRenderer/Lights/DirectionalLight.h"
#include "HeatrayRenderer/Lights/PointLight.h"
#include "HeatrayRenderer/Lights/SpotLight.h"
#include "HeatrayRenderer/Materials/GlassMaterial.h"
#include "HeatrayRenderer/Materials/PhysicallyBasedMaterial.h"
#include "Utility/AABB.h"
#include "Utility/Log.h"
#include "Utility/TextureLoader.h"

#include "assimp/pbrmaterial.h"
#include "glm/glm/glm.hpp"
#include "glm/glm/gtc/constants.hpp"
#include "glm/glm/gtc/quaternion.hpp"
#include "glm/glm/gtc/matrix_transform.hpp"
#include "glm/glm/gtx/euler_angles.hpp"
#include "glm/glm/gtx/transform.hpp"

#include <assert.h>
#include <filesystem>
#include <memory>

class AssimpLogToCoutStream : public Assimp::LogStream
{
public:
    AssimpLogToCoutStream() {}
    virtual ~AssimpLogToCoutStream() {}
    virtual void write(const char* message) { LOG_INFO("Assimp: %s", message); }
};

glm::mat4x4 getFullTransform(aiScene const *scene, const aiString &nodeName, const bool convertToMeters)
{
    // Walk up the tree until we find the root, concatenating transforms along the way.
    aiNode* node = scene->mRootNode->FindNode(nodeName);
    assert(node);
    aiMatrix4x4 transform = node->mTransformation;

    node = node->mParent;
    while (node) {
        transform = node->mTransformation * transform;
        node = node->mParent;
    }

    // glm is column major, assimp is row major;
    glm::mat4x4 finalTransform = glm::mat4x4(transform.a1, transform.b1, transform.c1, transform.d1,
                                             transform.a2, transform.b2, transform.c2, transform.d2,
                                             transform.a3, transform.b3, transform.c3, transform.d3,
                                             transform.a4, transform.b4, transform.c4, transform.d4);
    
    if (convertToMeters) {
        // Centimeters to meters.
        finalTransform[3][0] *= 0.01f;
        finalTransform[3][1] *= 0.01f;
        finalTransform[3][2] *= 0.01f;
    }

    return finalTransform;
}

AssimpMeshProvider::AssimpMeshProvider(std::string filename, bool convertToMeters, std::shared_ptr<Lighting> lighting)
: MeshProvider(filename)
, m_filename(std::move(filename))
, m_convertToMeters(convertToMeters)
{
    LoadScene(m_filename, lighting);
}

void AssimpMeshProvider::ProcessNode(const aiScene * scene, const aiNode * node, const aiMatrix4x4 & parentTransform, int level)
{
    aiMatrix4x4 transform = parentTransform * node->mTransformation;

    // Set the mesh transforms for this node.
    for (unsigned int ii = 0; ii < node->mNumMeshes; ++ii) {
        // glm is column major, assimp is row major;
        glm::mat4x4 *submeshTransform = &m_submeshes[node->mMeshes[ii]].localTransform;
        *submeshTransform = glm::mat4x4(transform.a1, transform.b1, transform.c1, transform.d1,
                                        transform.a2, transform.b2, transform.c2, transform.d2,
                                        transform.a3, transform.b3, transform.c3, transform.d3,
                                        transform.a4, transform.b4, transform.c4, transform.d4);

        // Determine the transformed AABB for this node in order to calculate the final scene AABB.
        {
            aiAABB node_aabb = scene->mMeshes[node->mMeshes[ii]]->mAABB;
            glm::vec4 aabb_min = glm::vec4(node_aabb.mMin.x, node_aabb.mMin.y, node_aabb.mMin.z, 1.0f);
            glm::vec4 aabb_max = glm::vec4(node_aabb.mMax.x, node_aabb.mMax.y, node_aabb.mMax.z, 1.0f);

            m_sceneAABB.expand(*submeshTransform * aabb_min);
            m_sceneAABB.expand(*submeshTransform * aabb_max);
        }
    }
    
    for (unsigned int ii = 0; ii < node->mNumChildren; ++ii) {
        ProcessNode(scene, node->mChildren[ii], transform, level + 1);
    }
}

void AssimpMeshProvider::ProcessMesh(aiMesh const * mesh)
{
    Submesh submesh;

    if (mesh->HasPositions()) {
        LOG_INFO("Positions: %u", mesh->mNumVertices);
        VertexAttribute & attribute = submesh.vertexAttributes[submesh.vertexAttributeCount];
        attribute.usage = VertexAttributeUsage_Position;
        attribute.buffer = (int)m_vertexBuffers.size();
        attribute.componentCount = 3;
        attribute.size = sizeof(float);
        attribute.offset = 0;
        attribute.stride = 3 * sizeof(float);

        m_vertexBuffers.push_back(std::vector<float>());
        std::vector<float> & vertexBuffer = m_vertexBuffers.back();
        vertexBuffer.reserve(mesh->mNumVertices * 3);

        for (uint32_t iVertex = 0; iVertex < mesh->mNumVertices; ++iVertex) {
            glm::vec3 position(mesh->mVertices[iVertex].x, mesh->mVertices[iVertex].y, mesh->mVertices[iVertex].z);
            vertexBuffer.push_back(position.x);
            vertexBuffer.push_back(position.y);
            vertexBuffer.push_back(position.z);
        }

        ++submesh.vertexAttributeCount;
    }
    if (mesh->HasNormals()) {
        LOG_INFO("Normals: %u", mesh->mNumVertices);
        VertexAttribute & attribute = submesh.vertexAttributes[submesh.vertexAttributeCount];
        attribute.usage = VertexAttributeUsage_Normal;
        attribute.buffer = (int)m_vertexBuffers.size();
        attribute.componentCount = 3;
        attribute.size = sizeof(float);
        attribute.offset = 0;
        attribute.stride = 3 * sizeof(float);

        m_vertexBuffers.push_back(std::vector<float>());
        std::vector<float> & vertexBuffer = m_vertexBuffers.back();
        vertexBuffer.reserve(mesh->mNumVertices * 3);

        for (uint32_t iVertex = 0; iVertex < mesh->mNumVertices; ++iVertex) {
            glm::vec3 normal(mesh->mNormals[iVertex].x, mesh->mNormals[iVertex].y, mesh->mNormals[iVertex].z);
            vertexBuffer.push_back(normal.x);
            vertexBuffer.push_back(normal.y);
            vertexBuffer.push_back(normal.z);
        }

        ++submesh.vertexAttributeCount;
    }
    if (mesh->HasTextureCoords(0)) {
        LOG_INFO("UVs: %u", mesh->mNumVertices);
        size_t positionsByteCount = mesh->mNumVertices * mesh->mNumUVComponents[0] * sizeof(float);

        VertexAttribute & attribute = submesh.vertexAttributes[submesh.vertexAttributeCount];
        attribute.usage = VertexAttributeUsage_TexCoord;
        attribute.buffer = (int)m_vertexBuffers.size();
        attribute.componentCount = mesh->mNumUVComponents[0];
        attribute.size = sizeof(float);
        attribute.offset = 0;
        attribute.stride = mesh->mNumUVComponents[0] * sizeof(float);

        m_vertexBuffers.push_back(std::vector<float>());
        std::vector<float> & vertexBuffer = m_vertexBuffers.back();
        vertexBuffer.reserve(mesh->mNumVertices * mesh->mNumUVComponents[0]);

        for (uint32_t iVertex = 0; iVertex < mesh->mNumVertices; ++iVertex) {
            for (uint32_t iComponent = 0; iComponent < mesh->mNumUVComponents[0]; ++iComponent) {
                vertexBuffer.push_back(mesh->mTextureCoords[0][iVertex][iComponent]);
            }
        }

        ++submesh.vertexAttributeCount;
    }
    if (mesh->HasTangentsAndBitangents()) {
        LOG_INFO("Tangents/Bitangents: %u", mesh->mNumVertices);
        // Tangents.
        {
            VertexAttribute& attribute = submesh.vertexAttributes[submesh.vertexAttributeCount];
            attribute.usage = VertexAttributeUsage_Tangents;
            attribute.buffer = (int)m_vertexBuffers.size();
            attribute.componentCount = 3;
            attribute.size = sizeof(float);
            attribute.offset = 0;
            attribute.stride = 3 * sizeof(float);

            m_vertexBuffers.push_back(std::vector<float>());
            std::vector<float>& vertexBuffer = m_vertexBuffers.back();
            vertexBuffer.reserve(mesh->mNumVertices * 3);

            for (uint32_t iVertex = 0; iVertex < mesh->mNumVertices; ++iVertex) {
                glm::vec3 tangent(mesh->mTangents[iVertex].x, mesh->mTangents[iVertex].y, mesh->mTangents[iVertex].z);
                vertexBuffer.push_back(tangent.x);
                vertexBuffer.push_back(tangent.y);
                vertexBuffer.push_back(tangent.z);
            }

            ++submesh.vertexAttributeCount;
        }

        // Bitangents.
        {
            VertexAttribute& attribute = submesh.vertexAttributes[submesh.vertexAttributeCount];
            attribute.usage = VertexAttributeUsage_Bitangents;
            attribute.buffer = (int)m_vertexBuffers.size();
            attribute.componentCount = 3;
            attribute.size = sizeof(float);
            attribute.offset = 0;
            attribute.stride = 3 * sizeof(float);

            m_vertexBuffers.push_back(std::vector<float>());
            std::vector<float>& vertexBuffer = m_vertexBuffers.back();
            vertexBuffer.reserve(mesh->mNumVertices * 3);

            for (uint32_t iVertex = 0; iVertex < mesh->mNumVertices; ++iVertex) {
                // Assimp often gives garbage bitangents, so we calculate our own here.
                glm::vec3 normal(mesh->mNormals[iVertex].x, mesh->mNormals[iVertex].y, mesh->mNormals[iVertex].z);
                glm::vec3 tangent(mesh->mTangents[iVertex].x, mesh->mTangents[iVertex].y, mesh->mTangents[iVertex].z);
                glm::vec3 bitangent = glm::cross(normal, tangent);
                vertexBuffer.push_back(bitangent.x);
                vertexBuffer.push_back(bitangent.y);
                vertexBuffer.push_back(bitangent.z);
            }

            ++submesh.vertexAttributeCount;
        }
    }

    if (mesh->HasVertexColors(0)) {
        LOG_INFO("Vertex Colors: %u", mesh->mNumVertices);
        VertexAttribute& attribute = submesh.vertexAttributes[submesh.vertexAttributeCount];
        attribute.usage = VertexAttributeUsage_Colors;
        attribute.buffer = (int)m_vertexBuffers.size();
        attribute.componentCount = 3;
        attribute.size = sizeof(float);
        attribute.offset = 0;
        attribute.stride = 3 * sizeof(float);

        m_vertexBuffers.push_back(std::vector<float>());
        std::vector<float>& vertexBuffer = m_vertexBuffers.back();
        vertexBuffer.reserve(mesh->mNumVertices * 3);

        for (uint32_t iVertex = 0; iVertex < mesh->mNumVertices; ++iVertex) {
            glm::vec3 color(mesh->mColors[0][iVertex].r, mesh->mColors[0][iVertex].g, mesh->mColors[0][iVertex].b);
            vertexBuffer.push_back(color.x);
            vertexBuffer.push_back(color.y);
            vertexBuffer.push_back(color.z);
        }

        ++submesh.vertexAttributeCount;

        // Tell the associated material to enable vertex colors.
        m_materials[mesh->mMaterialIndex]->enableVertexColors();
    }

    size_t indexCount = 0;
    for (unsigned int ff = 0; ff < mesh->mNumFaces; ++ff) {
        indexCount += (mesh->mFaces[ff].mNumIndices - 2) * 3;
    }

    m_indexBuffers.push_back(std::vector<int>());
    std::vector<int> & indexBuffer = m_indexBuffers.back();
    indexBuffer.reserve(indexCount);

    LOG_INFO("Building vertex indices...");
    for (unsigned int iFace = 0; iFace < mesh->mNumFaces; ++iFace) {
        auto & face = mesh->mFaces[iFace];

        for (unsigned int iSubFace = 2; iSubFace < face.mNumIndices; ++iSubFace) {
            indexBuffer.push_back(face.mIndices[0]);
            indexBuffer.push_back(face.mIndices[iSubFace-1]);
            indexBuffer.push_back(face.mIndices[iSubFace]);
        }
    }
    LOG_INFO("\tDONE");

    submesh.drawMode = DrawMode::Triangles;
    submesh.elementCount = indexCount;
    submesh.indexBuffer = m_indexBuffers.size() - 1;
    submesh.indexOffset = 0;
    submesh.materialIndex = mesh->mMaterialIndex;
    submesh.name = std::string(mesh->mName.C_Str());
    m_submeshes.push_back(submesh);
}

void AssimpMeshProvider::ProcessGlassMaterial(aiMaterial const* material)
{
    std::string name = material->GetName().C_Str();
    std::shared_ptr<GlassMaterial> glassMaterial = std::make_shared<GlassMaterial>(name);
    GlassMaterial::Parameters& params = glassMaterial->parameters();
    params.baseColor = { 1.0f, 1.0f, 1.0f };
    params.density = 0.05f;
    params.ior = 1.33f;
    params.roughness = 0.0f;

    material->Get(AI_MATKEY_ROUGHNESS_FACTOR, params.roughness);
    material->Get(AI_MATKEY_REFRACTI, params.ior);

    aiColor3D color;
    if (material->Get(AI_MATKEY_BASE_COLOR, color) == aiReturn_SUCCESS) {
        params.baseColor = glm::vec3(color.r, color.g, color.b);
    }

    // Textures.
    {
        auto filePath = std::filesystem::path(m_filename);
        auto fileParent = filePath.parent_path();

        // We overlap the loading of texture data off disk with the generation of
        // mipmaps from the most recently loaded texture.
        struct TextureTask {
            std::future<util::LoadedTexture> future;
            std::shared_ptr<openrl::Texture>* texture = nullptr;
        };

        TextureTask textureTasks[2];
        enum TaskID {
            kStaging = 0, // Texture task has been launched.
            kLoading      // Texture is loading or possibly fully loaded.
        };
        auto checkTextureTasks = [&textureTasks]() {
            if (textureTasks[kLoading].texture) {
                util::LoadedTexture loadedTexture = textureTasks[kLoading].future.get();
                if (loadedTexture.pixels) {
                    *(textureTasks[kLoading].texture) = openrl::Texture::create(loadedTexture.pixels.get(), loadedTexture.desc, loadedTexture.sampler, true);
                }
                textureTasks[kLoading].texture = nullptr;
            }
        };

        aiString assimpPath;
        if (material->GetTexture(AI_MATKEY_BASE_COLOR_TEXTURE, &assimpPath) == aiReturn_SUCCESS) {
            auto texturePath = (fileParent / assimpPath.C_Str()).string();
            textureTasks[kStaging].future = util::loadTextureAsync(texturePath.c_str(), true, true);
            textureTasks[kStaging].texture = &params.baseColorTexture;
            std::swap(textureTasks[kStaging], textureTasks[kLoading]);
        }
        else if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            material->GetTexture(aiTextureType_DIFFUSE, 0, &assimpPath);
            auto texturePath = (fileParent / assimpPath.C_Str()).string();
            textureTasks[kStaging].future = util::loadTextureAsync(texturePath.c_str(), true, true);
            textureTasks[kStaging].texture = &params.baseColorTexture;
            std::swap(textureTasks[kStaging], textureTasks[kLoading]);
        }

        if (material->GetTextureCount(aiTextureType_NORMALS) > 0) {
            aiString normalTexturePath;
            material->GetTexture(aiTextureType_NORMALS, 0, &normalTexturePath);
            auto texturePath = (fileParent / normalTexturePath.C_Str()).string();
            textureTasks[kStaging].future = util::loadTextureAsync(texturePath.c_str(), true, false);
            textureTasks[kStaging].texture = &params.normalmap;
            checkTextureTasks();
            std::swap(textureTasks[kStaging], textureTasks[kLoading]);
        }

        if (material->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE, &assimpPath) == aiReturn_SUCCESS) {
            auto texturePath = (fileParent / assimpPath.C_Str()).string();
            textureTasks[kStaging].future = util::loadTextureAsync(texturePath.c_str(), true, false);
            textureTasks[kStaging].texture = &params.metallicRoughnessTexture;
            checkTextureTasks();
            std::swap(textureTasks[kStaging], textureTasks[kLoading]);
        }

        // Ensure the final loaded texture gets uploaded to OpenRL.
        checkTextureTasks();
    }

    m_materials.push_back(glassMaterial);
}

void AssimpMeshProvider::ProcessMaterial(aiMaterial const * material)
{
    // Check to see if we should be processing this material as glass.
    {
        aiString mode;
        float transmissionFactor = 0.0f;
        material->Get(AI_MATKEY_GLTF_ALPHAMODE, mode);
        material->Get(AI_MATKEY_TRANSMISSION_FACTOR, transmissionFactor);
        if ((strcmp(mode.C_Str(), "BLEND") == 0) ||
            (transmissionFactor != 0.0f)) {
            // This is a transparent material.
            ProcessGlassMaterial(material);
            return;
        }
    }

    std::string name = material->GetName().C_Str();
    std::shared_ptr<PhysicallyBasedMaterial> pbrMaterial = std::make_shared<PhysicallyBasedMaterial>(name);
    PhysicallyBasedMaterial::Parameters& params = pbrMaterial->parameters();

    params.metallic = 0.0f;
    params.roughness = 1.0f;
    params.baseColor = { 1.0f, 1.0f, 1.0f };
    params.specularF0 = 0.5f;

    // Check to see if this material wants to perform alpha masking.
    {
        aiString mode;
        material->Get(AI_MATKEY_GLTF_ALPHAMODE, mode);
        if (strcmp(mode.C_Str(), "MASK") == 0) {
            params.alphaMask = true;
        }
    }

    aiColor3D color;
    if (material->Get(AI_MATKEY_BASE_COLOR, color) == aiReturn_SUCCESS) {
        params.baseColor = glm::vec3(color.r, color.g, color.b);
    } else if (material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == aiReturn_SUCCESS) {
        params.baseColor = glm::vec3(color.r, color.g, color.b);
    } else if (material->Get(AI_MATKEY_COLOR_SPECULAR, color) == aiReturn_SUCCESS) {
        params.baseColor = glm::vec3(color.r, color.g, color.b);
    }

    material->Get(AI_MATKEY_METALLIC_FACTOR, params.metallic);
    material->Get(AI_MATKEY_ROUGHNESS_FACTOR, params.roughness);
    material->Get(AI_MATKEY_CLEARCOAT_FACTOR, params.clearCoat);
    material->Get(AI_MATKEY_CLEARCOAT_ROUGHNESS_FACTOR, params.clearCoatRoughness);
    if (material->Get(AI_MATKEY_COLOR_EMISSIVE, color) == aiReturn_SUCCESS) {
        params.emissiveColor = glm::vec3(color.r, color.g, color.b);
    }
    material->Get(AI_MATKEY_TWOSIDED, params.doubleSided);

    {
        float ior = 0.0f;
        if (material->Get(AI_MATKEY_REFRACTI, ior) == aiReturn_SUCCESS) {
            params.specularF0 = std::powf((1.0f - ior) / (1.0f + ior), 2.0f);
        } else {
            material->Get(AI_MATKEY_SPECULAR_FACTOR, params.specularF0);
        }
    }

    auto filePath = std::filesystem::path(m_filename);
    auto fileParent = filePath.parent_path();

    // We overlap the loading of texture data off disk with the generation of
    // mipmaps from the most recently loaded texture.
    struct TextureTask {
        std::future<util::LoadedTexture> future;
        std::shared_ptr<openrl::Texture> *texture = nullptr;
    };

    TextureTask textureTasks[2];
    enum TaskID { 
        kStaging = 0, // Texture task has been launched.
        kLoading      // Texture is loading or possibly fully loaded.
    };
    auto checkTextureTasks = [&textureTasks]() {
        if (textureTasks[kLoading].texture) {
            util::LoadedTexture loadedTexture = textureTasks[kLoading].future.get();
            if (loadedTexture.pixels) {
                *(textureTasks[kLoading].texture) = openrl::Texture::create(loadedTexture.pixels.get(), loadedTexture.desc, loadedTexture.sampler, true);
            }
            textureTasks[kLoading].texture = nullptr;
        }
    };

    aiString fileTexturePath;
    if (material->GetTexture(AI_MATKEY_BASE_COLOR_TEXTURE, &fileTexturePath) == aiReturn_SUCCESS) {
        auto texturePath = (fileParent / fileTexturePath.C_Str()).string();
        textureTasks[kStaging].future = util::loadTextureAsync(texturePath.c_str(), true, true);
        textureTasks[kStaging].texture = &params.baseColorTexture;
        std::swap(textureTasks[kStaging], textureTasks[kLoading]);
    } else if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
        material->GetTexture(aiTextureType_DIFFUSE, 0, &fileTexturePath);
        auto texturePath = (fileParent / fileTexturePath.C_Str()).string();
        textureTasks[kStaging].future = util::loadTextureAsync(texturePath.c_str(), true, true);
        textureTasks[kStaging].texture = &params.baseColorTexture;
        std::swap(textureTasks[kStaging], textureTasks[kLoading]);
    }
    if (material->GetTextureCount(aiTextureType_EMISSIVE) > 0) {
        aiString emissiveTexturePath;
        material->GetTexture(aiTextureType_EMISSIVE, 0, &emissiveTexturePath);
        auto texturePath = (fileParent / emissiveTexturePath.C_Str()).string();
        textureTasks[kStaging].future = util::loadTextureAsync(texturePath.c_str(), true, true);
        textureTasks[kStaging].texture = &params.emissiveTexture;
        checkTextureTasks();
        std::swap(textureTasks[kStaging], textureTasks[kLoading]);
    }
    aiString fileRoughnessMetallicTexture;
    if (material->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE, &fileRoughnessMetallicTexture) == aiReturn_SUCCESS) {
        auto texturePath = (fileParent / fileRoughnessMetallicTexture.C_Str()).string();
        textureTasks[kStaging].future = util::loadTextureAsync(texturePath.c_str(), true, false);
        textureTasks[kStaging].texture = &params.metallicRoughnessTexture;
        checkTextureTasks();
        std::swap(textureTasks[kStaging], textureTasks[kLoading]);
    }
    if (material->GetTextureCount(aiTextureType_NORMALS) > 0) {
        aiString normalTexturePath;
        material->GetTexture(aiTextureType_NORMALS, 0, &normalTexturePath);
        auto texturePath = (fileParent / normalTexturePath.C_Str()).string();
        textureTasks[kStaging].future = util::loadTextureAsync(texturePath.c_str(), true, false);
        textureTasks[kStaging].texture = &params.normalmap;
        checkTextureTasks();
        std::swap(textureTasks[kStaging], textureTasks[kLoading]);
    }
    if (material->GetTexture(AI_MATKEY_CLEARCOAT_TEXTURE, &fileTexturePath) == aiReturn_SUCCESS) {
        auto texturePath = (fileParent / fileTexturePath.C_Str()).string();
        textureTasks[kStaging].future = util::loadTextureAsync(texturePath.c_str(), true, false);
        textureTasks[kStaging].texture = &params.clearCoatTexture;
        checkTextureTasks();
        std::swap(textureTasks[kStaging], textureTasks[kLoading]);
    }
    if (material->GetTexture(AI_MATKEY_CLEARCOAT_ROUGHNESS_TEXTURE, &fileTexturePath) == aiReturn_SUCCESS) {
        auto texturePath = (fileParent / fileTexturePath.C_Str()).string();
        textureTasks[kStaging].future = util::loadTextureAsync(texturePath.c_str(), true, false);
        textureTasks[kStaging].texture = &params.clearCoatRoughnessTexture;
        checkTextureTasks();
        std::swap(textureTasks[kStaging], textureTasks[kLoading]);
    }
    if (material->GetTexture(AI_MATKEY_CLEARCOAT_NORMAL_TEXTURE, &fileTexturePath) == aiReturn_SUCCESS) {
        auto texturePath = (fileParent / fileTexturePath.C_Str()).string();
        textureTasks[kStaging].future = util::loadTextureAsync(texturePath.c_str(), true, false);
        textureTasks[kStaging].texture = &params.clearCoatNormalmap;
        checkTextureTasks();
        std::swap(textureTasks[kStaging], textureTasks[kLoading]);
    }

    // Ensure the final loaded texture gets uploaded to OpenRL.
    checkTextureTasks();

    m_materials.push_back(pbrMaterial);
}

void AssimpMeshProvider::ProcessLight(aiLight const* light, std::shared_ptr<Lighting> lighting, const aiScene* scene)
{
    // Build the transform for this light following the scene all the way back to the root.
    // In Assimp, the only way to get a scene node for a given light is to search for the node by the name
    // of the light. This is why this isn't handled in ProcessNode().
    glm::mat4x4 lightTransform;
    {
        aiNode* node = scene->mRootNode->FindNode(light->mName);
        assert(node);
        aiMatrix4x4 transform = node->mTransformation;

        node = node->mParent;
        while (node) {
            transform = node->mTransformation * transform;
            node = node->mParent;
        }

        // glm is column major, assimp is row major;
        lightTransform = glm::mat4x4(transform.a1, transform.b1, transform.c1, transform.d1,
                                     transform.a2, transform.b2, transform.c2, transform.d2,
                                     transform.a3, transform.b3, transform.c3, transform.d3,
                                     transform.a4, transform.b4, transform.c4, transform.d4);
    }

    if (light->mType == aiLightSourceType::aiLightSource_POINT) {
        std::shared_ptr<PointLight> newLight = lighting->addPointLight(light->mName.C_Str());
        PointLight::Params params;

        // Light position.
        {
            glm::vec3 position = lightTransform * glm::vec4(light->mPosition.x, light->mPosition.y, light->mPosition.z, 1.0f);
            params.position = position;
        }

        // Light color/intensity. Assimp combines the intensity and color together so we
        // separate them back out here assuming that the color can not be greater than 1
        // for any channel.
        {
            glm::vec3 assimpColor = glm::vec3(light->mColorDiffuse.r, light->mColorDiffuse.g, light->mColorDiffuse.b);
            params.luminousIntensity = glm::length(assimpColor);
            params.color = assimpColor / params.luminousIntensity;
        }

        newLight->setParams(params);
        lighting->updateLight(newLight);
    } else if (light->mType == aiLightSource_DIRECTIONAL) {
        std::shared_ptr<DirectionalLight> newLight = lighting->addDirectionalLight(light->mName.C_Str());
        DirectionalLight::Params params;

        // Orientation.
        {
            // Extract the angles from the light's transform that define the light's final orientation.
            glm::vec4 yAxis = glm::vec4(light->mUp.x, light->mUp.y, light->mUp.z, 1.0f);
            glm::vec4 zAxis = glm::vec4(light->mDirection.x, light->mDirection.y, light->mDirection.z, 1.0f);
            glm::vec4 xAxis = glm::vec4(glm::cross(yAxis.xyz(), zAxis.xyz()), 1.0f);
            glm::mat4x4 localTransform = glm::mat4x4(xAxis, yAxis, zAxis, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

            glm::mat4x4 finalTransform = lightTransform * localTransform;
            float roll = 0.0f;
            float pitch = 0.0f;
            float yaw = 0.0f;
            glm::extractEulerAngleXYZ(finalTransform, pitch, yaw, roll);

            // Ensure that the angles are in range for the directional light.
            yaw = std::clamp(yaw, 0.0f, glm::two_pi<float>());
            pitch = std::clamp(pitch, -glm::half_pi<float>(), glm::half_pi<float>());

            // Angles are already in radians.
            params.orientation.phi = yaw;
            params.orientation.theta = pitch;
        }

        // Light color/intensity. Assimp combines the intensity and color together so we
        // separate them back out here assuming that the color can not be greater than 1
        // for any channel.
        {
            glm::vec3 assimpColor = glm::vec3(light->mColorDiffuse.r, light->mColorDiffuse.g, light->mColorDiffuse.b);
            params.illuminance = glm::length(assimpColor);
            params.color = assimpColor / params.illuminance;
        }

        newLight->setParams(params);
        lighting->updateLight(newLight);
    } else if (light->mType == aiLightSource_SPOT) {
        std::shared_ptr<SpotLight> newLight = lighting->addSpotLight(light->mName.C_Str());
        SpotLight::Params params;

        // Light position.
        {
            glm::vec3 position = lightTransform * glm::vec4(light->mPosition.x, light->mPosition.y, light->mPosition.z, 1.0f);
            params.position = position;
        }

        // Orientation.
        {
            // Extract the angles from the light's transform that define the light's final orientation.
            glm::vec4 yAxis = glm::vec4(light->mUp.x, light->mUp.y, light->mUp.z, 1.0f);
            glm::vec4 zAxis = glm::vec4(light->mDirection.x, light->mDirection.y, light->mDirection.z, 1.0f);
            glm::vec4 xAxis = glm::vec4(glm::cross(yAxis.xyz(), zAxis.xyz()), 1.0f);
            glm::mat4x4 localTransform = glm::mat4x4(xAxis, yAxis, zAxis, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

            glm::mat4x4 finalTransform = lightTransform * localTransform;
            float roll = 0.0f;
            float pitch = 0.0f;
            float yaw = 0.0f;
            glm::extractEulerAngleXYZ(finalTransform, pitch, yaw, roll);

            // Ensure that the angles are in range for the directional light.
            yaw = std::clamp(yaw, 0.0f, glm::two_pi<float>());
            pitch = std::clamp(pitch, -glm::half_pi<float>(), glm::half_pi<float>());

            // Angles are already in radians.
            params.orientation.phi = yaw;
            params.orientation.theta = pitch;
        }

        // Light color/intensity. Assimp combines the intensity and color together so we
        // separate them back out here assuming that the color can not be greater than 1
        // for any channel.
        {
            glm::vec3 assimpColor = glm::vec3(light->mColorDiffuse.r, light->mColorDiffuse.g, light->mColorDiffuse.b);
            params.luminousIntensity = glm::length(assimpColor);
            params.color = assimpColor / params.luminousIntensity;
        }

        // Finally the angles that control the light cone.
        {
            params.innerAngle = light->mAngleInnerCone;
            params.outerAngle = light->mAngleOuterCone;
        }

        newLight->setParams(params);
        lighting->updateLight(newLight);
    }
}

void AssimpMeshProvider::LoadScene(std::string const & filename, std::shared_ptr<Lighting> lighting)
{
    static bool assimpLoggerInitialized = false;

    if (!assimpLoggerInitialized) {
        Assimp::DefaultLogger::create();
        assimpLoggerInitialized = true;
    }

    AssimpLogToCoutStream stream;
    Assimp::DefaultLogger::get()->attachStream(&stream, 0xFF);

    Assimp::Importer importer;
    unsigned int postProcessFlags =
    aiProcess_JoinIdenticalVertices |
    aiProcess_FixInfacingNormals    |
    aiProcess_GenUVCoords           |
    aiProcess_OptimizeMeshes        |
    aiProcess_CalcTangentSpace      |
    aiProcess_GenBoundingBoxes      |
    aiProcess_Triangulate           |
    aiProcess_GenSmoothNormals      |
    aiProcess_TransformUVCoords     |
    aiProcess_GlobalScale;

    // Tell Assimp to apply the proper scene scale based on what the user has requested.
    // NOTE: it's assumed that if the user wants to convert to meters (the Heatray default)
    // then the model is originally in centimeters.
    importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, m_convertToMeters ? 0.01f : 1.0f);

    const aiScene * scene = importer.ReadFile(filename.c_str(), postProcessFlags);
    LOG_INFO("Scene imported by assimp!");

    if (scene) {
        for (unsigned int ii = 0; ii < scene->mNumMaterials; ++ii) {
            aiMaterial const* material = scene->mMaterials[ii];
            LOG_INFO("Processing material %s", material->GetName().C_Str());
            ProcessMaterial(material);
        }

        for (unsigned int ii = 0; ii < scene->mNumMeshes; ++ii) {
            aiMesh const * mesh = scene->mMeshes[ii];
            LOG_INFO("Processing mesh %s", mesh->mName.C_Str());
            ProcessMesh(mesh);
        }

        for (unsigned int ii = 0; ii < scene->mNumLights; ++ii) {
            aiLight const * light = scene->mLights[ii];
            LOG_INFO("Processing light %s", light->mName.C_Str());
            ProcessLight(light, lighting, scene);
        }

        aiMatrix4x4 identity;
        LOG_INFO("Processing scene transforms...");
        ProcessNode(scene, scene->mRootNode, identity, 0);
        LOG_INFO("\tDONE");
    } else {
        LOG_ERROR("Error: No scene found in asset.\n");
    }
}
