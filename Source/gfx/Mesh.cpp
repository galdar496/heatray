//
//  Mesh.cpp
//  Heatray
//
//  Created by Cody White on 5/12/14.
//
//

#include "Mesh.h"
#include "../math/Vector.h"
#include "../utility/util.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <fstream>
#include <iostream>
#include <vector>

#include <assimp/DefaultLogger.hpp>
#include <assimp/Logger.hpp>

#if defined(_WIN32) || defined(_WIN64)
    #define sscanf sscanf_s
#endif

namespace gfx
{

///
/// Convert an assimp material into a Heatray material.
///
/// @param sourceMaterial The assimp material to convert.
/// @param destMaterial The destination material to convert into (OUT).
///
void ConvertMaterial(aiMaterial *sourceMaterial, gfx::Material &destMaterial)
{
    aiColor3D color;
    aiString string;

    // Name.
    sourceMaterial->Get(AI_MATKEY_NAME, string);
    destMaterial.name = string.data;

    if (destMaterial.name.find("Light") != std::string::npos)
    {
        // This material is going to be used as a light source.
        destMaterial.componentFlags.set(gfx::Material::LIGHT);
    }

    // Diffuse.
    if (sourceMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color) == aiReturn_SUCCESS)
    {
        destMaterial.diffuse = math::Vec3f(color.r, color.g, color.b);
        if (destMaterial.diffuse.Dot(destMaterial.diffuse))
        {
            destMaterial.componentFlags.set(gfx::Material::DIFFUSE);
        }
    }

    // Specular.
    if (sourceMaterial->Get(AI_MATKEY_COLOR_SPECULAR, color) == aiReturn_SUCCESS)
    {
        destMaterial.specular = math::Vec3f(color.r, color.g, color.b);
        if (destMaterial.specular.Dot(destMaterial.specular))
        {
            destMaterial.componentFlags.set(gfx::Material::SPECULAR);
        }

        // Roughness.
        sourceMaterial->Get(AI_MATKEY_SHININESS, destMaterial.roughness);
        destMaterial.roughness /= 4.0f; // Assimp multiplies this by 4.
    }

    // Transmissive.
    if (sourceMaterial->Get(AI_MATKEY_COLOR_TRANSPARENT, color) == aiReturn_SUCCESS)
    {
        destMaterial.transmissive = math::Vec3f(color.r, color.g, color.b);
        if (destMaterial.transmissive.Dot(destMaterial.transmissive))
        {
            destMaterial.componentFlags.set(gfx::Material::TRANSMISSIVE);
        }

        // Index of refraction.
        sourceMaterial->Get(AI_MATKEY_REFRACTI, destMaterial.indexOfRefraction);
    }

    // Diffuse texture.
    if (sourceMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0)
    {
        sourceMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &string);
        destMaterial.diffuseTexture.LoadTextureData(string.data, true);
        destMaterial.componentFlags.set(gfx::Material::DIFFUSE_TEXTURE);
    }

    // Normalmap.
    if (sourceMaterial->GetTextureCount(aiTextureType_NORMALS) > 0)
    {
        sourceMaterial->GetTexture(aiTextureType_NORMALS, 0, &string);
        destMaterial.normalTexture.LoadTextureData(string.data, true);
        destMaterial.componentFlags.set(gfx::Material::NORMALMAP);
    }
}

class AssimpStream : public Assimp::LogStream
{
public:
    AssimpStream() {}
    virtual ~AssimpStream() {}
    virtual void write(const char *message) { std::cout << message << std::endl; }
};

Mesh::Mesh() :
    m_meshName("Unnamed Mesh")
{
}
    
Mesh::~Mesh()
{
    Destroy();
}

bool Mesh::Load(const std::string &filename, bool createRenderData)
{
    m_meshName = filename;
    AssimpStream stream;
    Assimp::DefaultLogger::create();
    Assimp::DefaultLogger::get()->attachStream(&stream, 0xFF);

    Assimp::Importer importer;
    unsigned int postProcessFlags = aiProcess_CalcTangentSpace      |
                                    aiProcess_JoinIdenticalVertices |
                                    aiProcess_Triangulate           |
                                    aiProcess_FixInfacingNormals    |
                                    aiProcess_FindInvalidData       |
                                    aiProcess_GenUVCoords           |
                                    aiProcess_OptimizeMeshes        |
                                    aiProcess_OptimizeGraph;
    const aiScene *scene = importer.ReadFile(filename, postProcessFlags);
    if (scene == nullptr)
    {
        std::cout << "Mesh::Load() - Could not open mesh file: " << filename << std::endl;
        return false;
    }

    // Get the triangle data from assimp.
    m_meshes.resize(scene->mNumMeshes);
    for (size_t ii = 0; ii < m_meshes.size(); ++ii)
    {
        aiMesh *mesh     = scene->mMeshes[ii];
        MeshPiece *piece = &m_meshes[ii];

        // Load the material for this mesh.
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
        ConvertMaterial(scene->mMaterials[mesh->mMaterialIndex], piece->material);

        // Read the buffers for each piece of mesh data.
        size_t numVertices = mesh->mNumVertices;

        if (mesh->HasPositions())
        {
            piece->vertices.resize(numVertices);
            memcpy(&(piece->vertices[0]), mesh->mVertices, sizeof(aiVector3D) * numVertices);
        }
        else
        {
            std::cout << "Mesh " << piece->material.name << " is missing vertices." << std::endl;
        }

        if (mesh->HasNormals())
        {
            piece->normals.resize(numVertices);
            memcpy(&(piece->normals[0]), mesh->mNormals, sizeof(aiVector3D) * numVertices);
        }
        else
        {
            std::cout << "Mesh " << piece->material.name << " is missing normals." << std::endl;
            return false;
        }

        if (mesh->HasTextureCoords(0))
        {
            piece->texCoords.resize(numVertices);

            // Have to copy tex coords manually since assimp stores the tex coords as 3-component vectors.
            for (size_t texCoordIndex = 0; texCoordIndex < numVertices; ++texCoordIndex)
            {
                piece->texCoords[texCoordIndex].v[0] = mesh->mTextureCoords[0][texCoordIndex].x;
                piece->texCoords[texCoordIndex].v[1] = mesh->mTextureCoords[0][texCoordIndex].y;
            }
        }
        // It's ok to not have tex coordinates.

        if (mesh->HasTangentsAndBitangents())
        {
            piece->tangents.resize(numVertices);
            piece->bitangents.resize(numVertices);
            memcpy(&(piece->tangents[0]), mesh->mTangents, sizeof(aiVector3D) * numVertices);
            memcpy(&(piece->bitangents[0]), mesh->mBitangents, sizeof(aiVector3D) * numVertices);
        }
        // It's ok to not have tangents and bitangents.

        // Extract the indices from the mesh manually (they aren't stored in a buffer).
        if (mesh->HasFaces())
        {
            // Only support triangles.
            assert(mesh->mFaces[0].mNumIndices == 3);

            size_t numIndices = mesh->mNumFaces * mesh->mFaces[0].mNumIndices;
            piece->indices.resize(numIndices);
            size_t counter = 0;

            for (size_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex)
            {
                const aiFace &face = mesh->mFaces[faceIndex];
                piece->indices[counter++] = face.mIndices[0];
                piece->indices[counter++] = face.mIndices[1];
                piece->indices[counter++] = face.mIndices[2];
            }
        }
        else
        {
            std::cout << "Mesh " << piece->material.name << " is missing faces." << std::endl;
            return false;
        }
    }

    if (createRenderData)
    {
        CreateRenderData();
    }

    return true;
}
    
void Mesh::Destroy()
{
    for (MeshList::iterator piece = m_meshes.begin(); piece != m_meshes.end(); ++piece)
    {
        if (piece->primitive != RL_NULL_PRIMITIVE)
        {
            rlDeletePrimitives(1, &piece->primitive);
            piece->primitive = RL_NULL_PRIMITIVE;
        }
        
        for (int ii = 0; ii < kNumVBOTypes; ++ii)
        {
            piece->buffers[ii].Destroy();
        }
        
        piece->program.Destroy();
        piece->material.Destroy();
    }
    
    ClearLoadedData();
}
    
void Mesh::ClearLoadedData()
{
    for (MeshList::iterator piece = m_meshes.begin(); piece != m_meshes.end(); ++piece)
    {
        piece->vertices.clear();
        piece->normals.clear();
        piece->texCoords.clear();
        piece->tangents.clear();
        piece->bitangents.clear();
        piece->indices.clear();
    }
}
    
Mesh::MeshList &Mesh::GetMeshList()
{
    return m_meshes;
}
    
std::string Mesh::GetName() const
{
	return m_meshName;
}
    
void Mesh::CreateRenderData()
{
	// Allocate a VBO for each material found.
    for (MeshList::iterator piece = m_meshes.begin(); piece != m_meshes.end(); ++piece)
	{
        // Generate the primitive for this mesh piece.
        rlGenPrimitives(1, &(piece->primitive));
        rlBindPrimitive(RL_PRIMITIVE, piece->primitive);
        rlPrimitiveParameterString(RL_PRIMITIVE, RL_PRIMITIVE_NAME, piece->material.name.c_str());
        rlBindPrimitive(RL_PRIMITIVE, RL_NULL_PRIMITIVE);

        // Generate any textures that this mesh needs.
        piece->material.diffuseTexture.CreateFromLoadedData(true);
        piece->material.normalTexture.CreateFromLoadedData(true);

        size_t numElements = piece->vertices.size();
        
        // Create and load the VBOs.
        piece->buffers[kVertices].Load(&(piece->vertices[0]), numElements * sizeof(math::Vec3f), "positions");
        piece->buffers[kNormals].Load(&(piece->normals[0]), numElements * sizeof(math::Vec3f), "normals");
        piece->buffers[kIndices].Load(&(piece->indices[0]), piece->indices.size() * sizeof(int), "indices");

        if (piece->texCoords.size())
        {
            // If there are tex coords then there will be tangents and bitangents as well.
            piece->buffers[kTexCoords].Load(&(piece->texCoords[0]), numElements * sizeof(math::Vec2f), "tex coords");
            piece->buffers[kTangents].Load(&(piece->tangents[0]), numElements * sizeof(math::Vec3f), "tangents");
            piece->buffers[kBitangents].Load(&(piece->bitangents[0]), numElements * sizeof(math::Vec3f), "bitangents");
        }
        
        piece->numIndices = piece->indices.size();
	}
}
    
} // namespace gfx
