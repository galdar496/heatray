#include "HeatrayRenderer/Materials/GlassMaterial.h"
#include "HeatrayRenderer/Materials/PhysicallyBasedMaterial.h"
#include "HeatrayRenderer/MeshProviders/AssimpMeshProvider.h"
#include "Utility/AABB.h"
#include "Utility/Log.h"
#include "Utility/TextureLoader.h"

#include "assimp/pbrmaterial.h"
#include "glm/glm/glm.hpp"
#include "glm/glm/gtc/constants.hpp"
#include "glm/glm/gtc/quaternion.hpp"
#include "glm/glm/gtc/matrix_transform.hpp"
#include "glm/glm/gtx/transform.hpp"

#include <filesystem>
#include <memory>

class AssimpLogToCoutStream : public Assimp::LogStream
{
public:
	AssimpLogToCoutStream() {}
	virtual ~AssimpLogToCoutStream() {}
	virtual void write(const char* message) { LOG_INFO("Assimp: %s", message); }
};

AssimpMeshProvider::AssimpMeshProvider(std::string filename, bool convert_to_meters, bool swapYZ)
: m_filename(std::move(filename)), m_swapYZ(swapYZ)
{
    LoadModel(m_filename, convert_to_meters);
}

void AssimpMeshProvider::ProcessNode(const aiScene * scene, const aiNode * node, const aiMatrix4x4 & parentTransform, int level, bool convert_to_meters)
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
		if (convert_to_meters) {
			// Centimeters to meters.
			(*submeshTransform)[3][0] *= 0.01f;
			(*submeshTransform)[3][1] *= 0.01f;
			(*submeshTransform)[3][2] *= 0.01f;
		}

		// Determine the transformed AABB for this node in order to calculate the final scene AABB.
		{
			aiAABB node_aabb = scene->mMeshes[node->mMeshes[ii]]->mAABB;
			glm::vec4 aabb_min = glm::vec4(node_aabb.mMin.x, node_aabb.mMin.y, node_aabb.mMin.z, 1.0f);
			glm::vec4 aabb_max = glm::vec4(node_aabb.mMax.x, node_aabb.mMax.y, node_aabb.mMax.z, 1.0f);

			// HACK! Sometimes there is a scale applied to the transform matrix. If we're doing a conversion to
			// meters just ignore this for now.
			glm::mat4x4 transform = *submeshTransform;
			if (convert_to_meters) {
				transform = glm::scale(glm::vec3(0.01f)) * transform;
			}

			m_sceneAABB.expand(transform * aabb_min);
			m_sceneAABB.expand(transform * aabb_max);
		}
    }
    
    for (unsigned int ii = 0; ii < node->mNumChildren; ++ii) {
        ProcessNode(scene, node->mChildren[ii], transform, level + 1, convert_to_meters);
    }
}

void AssimpMeshProvider::ProcessMesh(aiMesh const * mesh, bool convert_to_meters)
{
    Submesh submesh;

    if (mesh->HasPositions()) {
		LOG_INFO("Positions: %u", mesh->mNumVertices);
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

        for (uint32_t iVertex = 0; iVertex < mesh->mNumVertices; ++iVertex) {
            glm::vec3 position(mesh->mVertices[iVertex].x, mesh->mVertices[iVertex].y, mesh->mVertices[iVertex].z);
            if (m_swapYZ) {
                position = glm::vec3(position.x, position.z, -position.y);
            }
			if (convert_to_meters) {
				position *= 0.01f; // Centimeters to meters.
			}
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
        attribute.buffer = m_vertexBuffers.size();
        attribute.componentCount = 3;
        attribute.size = sizeof(float);
        attribute.offset = 0;
        attribute.stride = 3 * sizeof(float);

        m_vertexBuffers.push_back(std::vector<float>());
        std::vector<float> & vertexBuffer = m_vertexBuffers.back();
        vertexBuffer.reserve(mesh->mNumVertices * 3);

        for (uint32_t iVertex = 0; iVertex < mesh->mNumVertices; ++iVertex) {
            glm::vec3 normal(mesh->mNormals[iVertex].x, mesh->mNormals[iVertex].y, mesh->mNormals[iVertex].z);
            if (m_swapYZ) {
                normal = glm::vec3(normal.x, normal.z, -normal.y);
            }
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
        attribute.buffer = m_vertexBuffers.size();
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
			attribute.buffer = m_vertexBuffers.size();
			attribute.componentCount = 3;
			attribute.size = sizeof(float);
			attribute.offset = 0;
			attribute.stride = 3 * sizeof(float);

			m_vertexBuffers.push_back(std::vector<float>());
			std::vector<float>& vertexBuffer = m_vertexBuffers.back();
			vertexBuffer.reserve(mesh->mNumVertices * 3);

			for (uint32_t iVertex = 0; iVertex < mesh->mNumVertices; ++iVertex) {
				glm::vec3 tangent(mesh->mTangents[iVertex].x, mesh->mTangents[iVertex].y, mesh->mTangents[iVertex].z);
				if (m_swapYZ) {
					tangent = glm::vec3(tangent.x, tangent.z, -tangent.y);
				}
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
			attribute.buffer = m_vertexBuffers.size();
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
				if (m_swapYZ) {
					bitangent = glm::vec3(bitangent.x, bitangent.z, -bitangent.y);
				}
				vertexBuffer.push_back(bitangent.x);
				vertexBuffer.push_back(bitangent.y);
				vertexBuffer.push_back(bitangent.z);
			}

			++submesh.vertexAttributeCount;
		}
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
    m_submeshes.push_back(submesh);
}

void AssimpMeshProvider::ProcessGlassMaterial(aiMaterial const* material)
{
    GlassMaterial::Parameters params;
    params.baseColor = { 1.0f, 1.0f, 1.0f };
    params.density = 0.05f;
    params.ior = 1.33f;
    params.roughness = 0.0f;

    material->Get(AI_MATKEY_ROUGHNESS_FACTOR, params.roughness);
	material->Get(AI_MATKEY_REFRACTI, params.ior);

    aiColor3D color;
	material->Get(AI_MATKEY_BASE_COLOR, color);
    params.baseColor = glm::vec3(color.r, color.g, color.b);

	std::shared_ptr<GlassMaterial> glassMaterial = std::make_shared<GlassMaterial>();
    glassMaterial->build(params);
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

    PhysicallyBasedMaterial::Parameters params;

    params.metallic = 0.0f;
    params.roughness = 1.0f;
    params.baseColor = { 1.0f, 1.0f, 1.0f };
    params.specularF0 = 0.5f;

    aiColor3D color;
    if (material->Get(AI_MATKEY_BASE_COLOR, color) == aiReturn_SUCCESS) {
        params.baseColor = glm::vec3(color.r, color.g, color.b);
    }

    material->Get(AI_MATKEY_METALLIC_FACTOR, params.metallic);
    material->Get(AI_MATKEY_ROUGHNESS_FACTOR, params.roughness);
	material->Get(AI_MATKEY_CLEARCOAT_FACTOR, params.clearCoat);
	material->Get(AI_MATKEY_CLEARCOAT_ROUGHNESS_FACTOR, params.clearCoatRoughness);

	{
		float ior = 0.0f;
		if (material->Get(AI_MATKEY_REFRACTI, ior) == aiReturn_SUCCESS) {
			params.specularF0 = std::powf((1.0f - ior) / (1.0f + ior), 2.0f);
		}
	}

    auto filePath = std::filesystem::path(m_filename);
    auto fileParent = filePath.parent_path();

    aiString fileTexturePath;
    if (material->GetTexture(AI_MATKEY_BASE_COLOR_TEXTURE, &fileTexturePath) == aiReturn_SUCCESS) {
        auto texturePath = (fileParent / fileTexturePath.C_Str()).string();
        params.baseColorTexture = std::make_shared<openrl::Texture>(util::loadTexture(texturePath.c_str(), true, true));
    } else if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
        material->GetTexture(aiTextureType_DIFFUSE, 0, &fileTexturePath);
        auto texturePath = (fileParent / fileTexturePath.C_Str()).string();
        params.baseColorTexture = std::make_shared<openrl::Texture>(util::loadTexture(texturePath.c_str(), true, true));
    }
	if (material->GetTextureCount(aiTextureType_EMISSIVE) > 0) {
		aiString emissiveTexturePath;
		material->GetTexture(aiTextureType_EMISSIVE, 0, &emissiveTexturePath);
		auto texturePath = (fileParent / emissiveTexturePath.C_Str()).string();
		params.emissiveTexture = std::make_shared<openrl::Texture>(util::loadTexture(texturePath.c_str(), true, true));
	}
    aiString fileRoughnessMetallicTexture;
    if (material->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE, &fileRoughnessMetallicTexture) == aiReturn_SUCCESS) {
        auto texturePath = (fileParent / fileRoughnessMetallicTexture.C_Str()).string();
        params.metallicRoughnessTexture = std::make_shared<openrl::Texture>(util::loadTexture(texturePath.c_str(), true, false));
    }
	if (material->GetTextureCount(aiTextureType_NORMALS) > 0) {
		aiString normalTexturePath;
		material->GetTexture(aiTextureType_NORMALS, 0, &normalTexturePath);
		auto texturePath = (fileParent / normalTexturePath.C_Str()).string();
		params.normalmap = std::make_shared<openrl::Texture>(util::loadTexture(texturePath.c_str(), true, false));
	}
	if (material->GetTexture(AI_MATKEY_CLEARCOAT_TEXTURE, &fileTexturePath) == aiReturn_SUCCESS) {
		auto texturePath = (fileParent / fileTexturePath.C_Str()).string();
		params.clearCoatTexture = std::make_shared<openrl::Texture>(util::loadTexture(texturePath.c_str(), true, false));
	}
	if (material->GetTexture(AI_MATKEY_CLEARCOAT_ROUGHNESS_TEXTURE, &fileTexturePath) == aiReturn_SUCCESS) {
		auto texturePath = (fileParent / fileTexturePath.C_Str()).string();
		params.clearCoatRoughnessTexture = std::make_shared<openrl::Texture>(util::loadTexture(texturePath.c_str(), true, false));
	}
	if (material->GetTexture(AI_MATKEY_CLEARCOAT_NORMAL_TEXTURE, &fileTexturePath) == aiReturn_SUCCESS) {
		auto texturePath = (fileParent / fileTexturePath.C_Str()).string();
		params.clearCoatNormalmap = std::make_shared<openrl::Texture>(util::loadTexture(texturePath.c_str(), true, false));
	}

	std::shared_ptr<PhysicallyBasedMaterial> pbrMaterial = std::make_shared<PhysicallyBasedMaterial>();
    pbrMaterial->build(params);
    m_materials.push_back(pbrMaterial);
}

void AssimpMeshProvider::LoadModel(std::string const & filename, bool convert_to_meters)
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
	aiProcess_GenBoundingBoxes;

    const aiScene * scene = importer.ReadFile(filename.c_str(), postProcessFlags);
	LOG_INFO("Scene imported by assimp!");

    if (scene) {
        for (unsigned int ii = 0; ii < scene->mNumMeshes; ++ii) {
            aiMesh const * mesh = scene->mMeshes[ii];
			LOG_INFO("Processing mesh %s", mesh->mName.C_Str());
            ProcessMesh(mesh, convert_to_meters);
        }

        for (unsigned int ii = 0; ii < scene->mNumMaterials; ++ii) {
            aiMaterial const * material = scene->mMaterials[ii];
			LOG_INFO("Processing material %s", material->GetName().C_Str());
            ProcessMaterial(material);
        }

        aiMatrix4x4 identity;
		LOG_INFO("Processing scene transforms...");
        ProcessNode(scene, scene->mRootNode, identity, 0, convert_to_meters);
		LOG_INFO("\tDONE");
    } else {
        printf("Error:  No scene found in asset.\n");
    }
}
