#include "HeatrayRenderer/Materials/PhysicallyBasedMaterial.h"
#include "HeatrayRenderer/MeshProviders/AssimpMeshProvider.h"
#include "Utility/TextureLoader.h"

#include "assimp/pbrmaterial.h"
#include "glm/glm/glm.hpp"
#include "glm/glm/gtc/constants.hpp"
#include "glm/glm/gtc/quaternion.hpp"

#include <filesystem>
#include <iostream>
#include <memory>

class AssimpLogToCoutStream : public Assimp::LogStream
{
public:
	AssimpLogToCoutStream() {}
	virtual ~AssimpLogToCoutStream() {}
	virtual void write(const char *message) { std::cout << "Assimp: " << message << std::endl; }
};

AssimpMeshProvider::AssimpMeshProvider(std::string filename, bool swapYZ)
: m_filename(std::move(filename)), m_swapYZ(swapYZ)
{
    LoadModel(m_filename);
}

void AssimpMeshProvider::ProcessNode(const aiScene * scene, const aiNode * node, const aiMatrix4x4 & parentTransform, int level)
{
    aiMatrix4x4 transform = parentTransform * node->mTransformation;
    
    for (unsigned int ii = 0; ii < node->mNumChildren; ++ii)
    {
        ProcessNode(scene, node->mChildren[ii], transform, level + 1);
    }
}

void AssimpMeshProvider::ProcessMesh(aiMesh const * mesh)
{
    Submesh submesh;

    if (mesh->HasPositions())
    {
        VertexAttribute & attribute = submesh.vertexAttributes[submesh.vertexAttributeCount];
        attribute.usage = VertexAttributeUsage_Position;
        attribute.buffer = m_vertexBuffers.size();
        attribute.componentCount = 3;
        attribute.size = sizeof(float);
        attribute.offset = 0;
        attribute.stride = 3 * sizeof(float);

        m_vertexBuffers.push_back(std::vector<float>());
        std::vector<float> & vertexBuffer = m_vertexBuffers.back();
        vertexBuffer.reserve(mesh->mNumVertices * 3);

        for (uint32_t iVertex = 0; iVertex < mesh->mNumVertices; ++iVertex)
        {
            glm::vec3 position(mesh->mVertices[iVertex].x, mesh->mVertices[iVertex].y, mesh->mVertices[iVertex].z);
            if (m_swapYZ)
            {
                position = glm::vec3(position.x, position.z, -position.y);
            }
            vertexBuffer.push_back(position.x);
            vertexBuffer.push_back(position.y);
            vertexBuffer.push_back(position.z);
        }

        ++submesh.vertexAttributeCount;
    }
    if (mesh->HasNormals())
    {
        VertexAttribute & attribute = submesh.vertexAttributes[submesh.vertexAttributeCount];
        attribute.usage = VertexAttributeUsage_Normal;
        attribute.buffer = m_vertexBuffers.size();
        attribute.componentCount = 3;
        attribute.size = sizeof(float);
        attribute.offset = 0;
        attribute.stride = 3 * sizeof(float);

        m_vertexBuffers.push_back(std::vector<float>());
        std::vector<float> & vertexBuffer = m_vertexBuffers.back();
        vertexBuffer.reserve(mesh->mNumVertices * 3);

        for (uint32_t iVertex = 0; iVertex < mesh->mNumVertices; ++iVertex)
        {
            glm::vec3 normal(mesh->mNormals[iVertex].x, mesh->mNormals[iVertex].y, mesh->mNormals[iVertex].z);
            if (m_swapYZ)
            {
                normal = glm::vec3(normal.x, normal.z, -normal.y);
            }
            vertexBuffer.push_back(normal.x);
            vertexBuffer.push_back(normal.y);
            vertexBuffer.push_back(normal.z);
        }

        ++submesh.vertexAttributeCount;
    }
    if (mesh->HasTextureCoords(0))
    {
        size_t positionsByteCount = mesh->mNumVertices * mesh->mNumUVComponents[0] * sizeof(float);

        VertexAttribute & attribute = submesh.vertexAttributes[submesh.vertexAttributeCount];
        attribute.usage = VertexAttributeUsage_TexCoord;
        attribute.buffer = m_vertexBuffers.size();
        attribute.componentCount = mesh->mNumUVComponents[0];
        attribute.size = sizeof(float);
        attribute.offset = 0;
        attribute.stride = mesh->mNumUVComponents[0] * sizeof(float);

        m_vertexBuffers.push_back(std::vector<float>());
        std::vector<float> & vertexBuffer = m_vertexBuffers.back();
        vertexBuffer.reserve(mesh->mNumVertices * mesh->mNumUVComponents[0]);

        for (uint32_t iVertex = 0; iVertex < mesh->mNumVertices; ++iVertex)
        {
            glm::vec3 normal(mesh->mNormals[iVertex].x, mesh->mNormals[iVertex].y, mesh->mNormals[iVertex].z);
            if (m_swapYZ)
            {
                normal = glm::vec3(normal.x, -normal.z, normal.y);
            }
            for (uint32_t iComponent = 0; iComponent < mesh->mNumUVComponents[0]; ++iComponent)
            {
                vertexBuffer.push_back(mesh->mTextureCoords[0][iVertex][iComponent]);
            }
        }

        ++submesh.vertexAttributeCount;
    }

    size_t indexCount = 0;
    for (unsigned int ff = 0; ff < mesh->mNumFaces; ++ff)
    {
        indexCount += (mesh->mFaces[ff].mNumIndices - 2) * 3;
    }

    m_indexBuffers.push_back(std::vector<int>());
    std::vector<int> & indexBuffer = m_indexBuffers.back();
    indexBuffer.reserve(indexCount);

    for (unsigned int iFace = 0; iFace < mesh->mNumFaces; ++iFace)
    {
        auto & face = mesh->mFaces[iFace];

        for (unsigned int iSubFace = 2; iSubFace < face.mNumIndices; ++iSubFace)
        {
            indexBuffer.push_back(face.mIndices[0]);
            indexBuffer.push_back(face.mIndices[iSubFace-1]);
            indexBuffer.push_back(face.mIndices[iSubFace]);
        }
    }

    submesh.drawMode = DrawMode::Triangles;
    submesh.elementCount = indexCount;
    submesh.indexBuffer = m_indexBuffers.size() - 1;
    submesh.indexOffset = 0;
    submesh.materialIndex = mesh->mMaterialIndex;
    m_submeshes.push_back(submesh);
}

void AssimpMeshProvider::ProcessMaterial(aiMaterial const * material)
{
    PhysicallyBasedMaterial::Parameters params;

    params.metallic = 0.0f;
    params.roughness = 1.0f;
    params.baseColor = {1, 1, 1};
    params.specularF0 = 0.5f;

    aiColor3D color;
    if (material->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_FACTOR, color) == aiReturn_SUCCESS)
    {
        params.baseColor = glm::vec3(color.r, color.g, color.b);
    }
    else if (material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == aiReturn_SUCCESS)
    {
        params.baseColor = glm::vec3(color.r, color.g, color.b);
    }

    material->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR, params.metallic);
    material->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, params.roughness);

    auto filePath = std::filesystem::path(m_filename);
    auto fileParent = filePath.parent_path();

    aiString fileBaseColor;
    if (material->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_TEXTURE, &fileBaseColor) == aiReturn_SUCCESS)
    {
        auto texturePath = (fileParent / fileBaseColor.C_Str()).string();
        params.baseColorTexture = std::make_shared<openrl::Texture>(util::loadTexture(texturePath.c_str(), true));
    }
    else if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
    {
        material->GetTexture(aiTextureType_DIFFUSE, 0, &fileBaseColor);
        auto texturePath = (fileParent / fileBaseColor.C_Str()).string();
        params.baseColorTexture = std::make_shared<openrl::Texture>(util::loadTexture(texturePath.c_str(), true));
    }

    PhysicallyBasedMaterial * pbrMaterial = new PhysicallyBasedMaterial();
    pbrMaterial->build(params);
    m_materials.push_back(pbrMaterial);
}

void AssimpMeshProvider::LoadModel(std::string const & filename)
{
    static bool assimpLoggerInitialized = false;

    if (!assimpLoggerInitialized)
    {
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
    aiProcess_CalcTangentSpace;

    const aiScene * scene = importer.ReadFile(filename.c_str(), postProcessFlags);
    
    if (scene)
    {
        for (unsigned int ii = 0; ii < scene->mNumMeshes; ++ii)
        {
            aiMesh const * mesh = scene->mMeshes[ii];
            ProcessMesh(mesh);
        }

        for (unsigned int ii = 0; ii < scene->mNumMaterials; ++ii)
        {
            aiMaterial const * material = scene->mMaterials[ii];
            ProcessMaterial(material);
        }

        aiMatrix4x4 identity;
        ProcessNode(scene, scene->mRootNode, identity, 0);
    }
    else
    {
        printf("Error:  No scene found in asset.\n");
    }
}
