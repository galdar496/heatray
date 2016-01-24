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
#include <fstream>
#include <iostream>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
    #define sscanf sscanf_s
#endif

namespace gfx
{

Mesh::Mesh() :
    m_meshName("Unnamed Mesh"),
    m_meshScale(1.0f)
{
}
    
Mesh::~Mesh()
{
    Destroy();
}
    
bool Mesh::Load(const std::string &filename, const bool createRenderData, const float scale, const bool clearData)
{
    // Extract the base path of 'filename'. This gives us a shared path between the mesh file and it's material file.
	std::string path = filename;
	path = path.substr(0, path.find_last_of("/") + 1);
    
	// Open the input file.
    std::ifstream fin;
	fin.open(filename);
	if (!fin)
	{
        std::cout << "Mesh::load() - Could not open obj file: " << filename << std::endl;
		return false;
	}
    
    m_meshName  = filename;
    m_meshScale = scale;
    
    // Add a default material for use if no material file is specified in the obj file.
    static const char *defaultMaterialName = "default**material*!@#$%^&*()";
    {
        Material defaultMaterial;
        defaultMaterial.name = defaultMaterialName;
        defaultMaterial.roughness = 0.0f;
        defaultMaterial.transmissive = math::Vec3f::Zero();
        defaultMaterial.specular = math::Vec3f::Zero();
        defaultMaterial.diffuse = math::Vec3f::Zero();
        defaultMaterial.indexOfRefraction = 1.0f;
        m_meshes[defaultMaterialName].material = defaultMaterial;
    }
    
    bool useNormals   = false;
    bool useTexCoords = false;
    
    std::vector<math::Vec3f> vertices;
    std::vector<math::Vec3f> normals;
    std::vector<math::Vec2f> texCoords;
    
    MeshList::iterator currentMesh = m_meshes.find(defaultMaterialName);
    
    // Read the input obj file.
    std::string inputLine = "";
	while (fin >> inputLine)
	{
		// The '#' character denotes a line comment.  Therefore, ignore everything after this character on the same line.
		if (inputLine[0] == '#')
		{
			char tmp[200];
			fin.getline(tmp, 200, '\n');
		}
        
		else if (inputLine == "mtllib")
		{
			// This is the material file, parse it.
			std::string materialFilename;
			fin >> materialFilename;
			materialFilename = path + materialFilename;
			m_meshes.erase(defaultMaterialName);  // Remove the default material because it is not needed.
            
			// Load the materials.
			if (!LoadMaterials(materialFilename, path, clearData, m_meshes))
            {
                fin.close();
                return false;
            }
		}
        
		// Vertex
		else if (inputLine == "v")
		{
			math::Vec3f vertex;
			fin >> vertex[0] >> vertex[1] >> vertex[2];
			vertex *= scale;
            vertices.push_back(vertex);
		}
        
		// Object name
		else if (inputLine == "o")
		{
			std::string name;
			fin >> name;
		}
        
		// Vertex normal
		else if (inputLine == "vn")
		{
			math::Vec3f normal;
			fin >> normal[0] >> normal[1] >> normal[2];
            normals.push_back(normal);
            useNormals = true;
		}
        
		// Texture coordinate
		else if (inputLine == "vt")
		{
			math::Vec2f texCoord;
			fin >> texCoord[0] >> texCoord[1];
            texCoords.push_back(texCoord);
            useTexCoords = true;
		}
        
		// Current material name.
		else if (inputLine == "usemtl")
		{
            std::string currentMaterial;
			fin >> currentMaterial;
            currentMesh = m_meshes.find(currentMaterial);
		}
        
		// Smooth
		else if (inputLine == "s")
		{
			std::string smooth;
			fin >> smooth;
		}
        
		// Triangle (face)
		else if (inputLine == "f")
		{
            math::Vec3i vertexIndices;
            math::Vec3i normalIndices;
            math::Vec3i texCoordIndices;
            
			std::string line;
			for (int i = 0; i < 3; ++i)
			{
				fin >> line;
				if (useNormals && !useTexCoords)
				{
					size_t cut = line.find("/");
					line[cut] = ' ';
					line[cut + 1] = ' ';
					sscanf(line.c_str(), "%d %d", &vertexIndices[i], &normalIndices[i]);
				}
				else if (!useNormals && useTexCoords)
				{
					size_t cut = line.find("/");
					line[cut] = ' ';
					sscanf(line.c_str(), "%d %d", &vertexIndices[i], &texCoordIndices[i]);
				}
				else if (!useNormals && !useTexCoords)
				{
					sscanf(line.c_str(), "%d", &vertexIndices[i]);
				}
				else if (useNormals && useTexCoords)
				{
					size_t cut = line.find("/");
					line[cut] = ' ';
					cut = line.find("/");
					line[cut] = ' ';
					sscanf(line.c_str(), "%d %d %d", &vertexIndices[i], &texCoordIndices[i], &normalIndices[i]);
				}
			}
            
            gfx::Triangle triangle;
            triangle.vertices[0] = vertices[vertexIndices[0] - 1];
            triangle.vertices[1] = vertices[vertexIndices[1] - 1];
            triangle.vertices[2] = vertices[vertexIndices[2] - 1];
            
			if (useNormals)
			{
                triangle.normals[0] = normals[normalIndices[0] - 1];
                triangle.normals[1] = normals[normalIndices[1] - 1];
                triangle.normals[2] = normals[normalIndices[2] - 1];
			}
            
			if (useTexCoords)
			{
                triangle.tex_coords[0] = texCoords[texCoordIndices[0] - 1];
                triangle.tex_coords[1] = texCoords[texCoordIndices[1] - 1];
                triangle.tex_coords[2] = texCoords[texCoordIndices[2] - 1];
                
                triangle.calculateTangents();
			}
            
            // Push the triangle information into the mesh data lists.
            for (int jj = 0; jj < 3; ++jj)
            {
                currentMesh->second.vertices.push_back(triangle.vertices[jj]);
                currentMesh->second.normals.push_back(triangle.normals[jj]);
                currentMesh->second.tangents.push_back(triangle.tangents[jj]);
                currentMesh->second.texCoords.push_back(triangle.tex_coords[jj]);
            }
            
		}
	}
    
	fin.close();
    
	if (createRenderData)
	{
		CreateRenderData();
	}
    
	if (clearData)
	{
		ClearLoadedData();
	}
    
    return true;
}
    
void Mesh::Destroy()
{
    for (MeshList::iterator iter = m_meshes.begin(); iter != m_meshes.end(); ++iter)
    {
        MeshPiece *piece = &(iter->second);
        if (piece->primitive != RL_NULL_PRIMITIVE)
        {
            rlDeletePrimitives(1, &piece->primitive);
            piece->primitive = RL_NULL_PRIMITIVE;
        }
        
        for (int ii = 0; ii < NUM_VBO_TYPES; ++ii)
        {
            piece->buffers[ii].Destroy();
        }
        
        piece->program.destroy();
        piece->material.Destroy();
    }
    
    ClearLoadedData();
}
    
void Mesh::ClearLoadedData()
{
    for (MeshList::iterator iter = m_meshes.begin(); iter != m_meshes.end(); ++iter)
    {
		iter->second.vertices.clear();
        iter->second.normals.clear();
        iter->second.texCoords.clear();
        iter->second.tangents.clear();
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

float Mesh::GetScale() const
{
    return m_meshScale;
}
    
bool Mesh::LoadMaterials(const std::string &filename, const std::string &basePath, const bool clearData, MeshList &materials)
{
    std::ifstream fin;
    fin.open(filename);
    if (!fin)
    {
        std::cout << "Mesh::loadMaterials() - Could not open material file: " << filename << std::endl;
        return false;
    }
    
    std::string materialName = "";
    std::string materialLine = "";
    MeshList::iterator materialIter;
    
    // Read the material file.
    while (fin >> materialLine)
    {
        if (materialLine[0] == '#')
        {
            char tmp[200];
            fin.getline(tmp, 200, '\n');
        }
        
        else if (materialLine == "newmtl")
        {
            fin >> materialName;
            materials[materialName].material.name = materialName;
            materialIter = materials.find(materialName);
            if (materialIter == materials.end())
            {
                std::cout << "Mesh::loadMaterials() - Could not insert material " << materialName << " into the material map." << std::endl;
                fin.close();
                return false;
            }

            if (materialName.find("Light") != std::string::npos)
            {
                // This material is going to be used as a light source.
                materialIter->second.material.componentFlags.set(gfx::Material::LIGHT);
            }
        }
        
        else if (materialLine == "Ns")
        {
            fin >> materialIter->second.material.roughness;
        }
        
        // We'll read Ka as the transmissive component, not the ambient.
        else if (materialLine == "Ka")
        {
            fin >> materialIter->second.material.transmissive[0] >> materialIter->second.material.transmissive[1] >> materialIter->second.material.transmissive[2];
            if (materialIter->second.material.transmissive != math::Vec3f::Zero())
            {
                materialIter->second.material.componentFlags.set(gfx::Material::TRANSMISSIVE);
            }
        }
        
        else if (materialLine == "Kd")
        {
            fin >> materialIter->second.material.diffuse[0] >> materialIter->second.material.diffuse[1] >> materialIter->second.material.diffuse[2];
            if (materialIter->second.material.diffuse != math::Vec3f::Zero())
            {
                materialIter->second.material.componentFlags.set(gfx::Material::DIFFUSE);
            }
        }
        
        else if (materialLine == "Ks")
        {
            fin >> materialIter->second.material.specular[0] >> materialIter->second.material.specular[1] >> materialIter->second.material.specular[2];
            if (materialIter->second.material.specular != math::Vec3f::Zero())
            {
                materialIter->second.material.componentFlags.set(gfx::Material::SPECULAR);
            }
        }
        
        else if (materialLine == "Ksub")
        {
            fin >> materialIter->second.material.diffuse[0] >> materialIter->second.material.diffuse[1] >> materialIter->second.material.diffuse[2];
            if (materialIter->second.material.diffuse != math::Vec3f::Zero())
            {
                materialIter->second.material.componentFlags.set(gfx::Material::SUBSURFACE);
                materialIter->second.material.componentFlags.set(gfx::Material::DIFFUSE);
            }
        }
        
        else if (materialLine == "Ni")
        {
            fin >> materialIter->second.material.indexOfRefraction;
        }
        
        // Unused.
        else if (materialLine == "d")
        {
            int dummy;
            fin >> dummy;
        }
        
        else if (materialLine == "map_Kd")
        {
            std::string texturePath;
            fin >> texturePath;
            
            texturePath = basePath + texturePath;
            materialIter->second.material.diffuseTexture.loadTextureData(texturePath);
            materialIter->second.material.componentFlags.set(gfx::Material::DIFFUSE_TEXTURE);
        }
        
        else if (materialLine == "map_Bump")
        {
            std::string texturePath;
            fin >> texturePath;
            
            texturePath = basePath + texturePath;
            materialIter->second.material.normalTexture.loadTextureData(texturePath);
            materialIter->second.material.componentFlags.set(gfx::Material::NORMALMAP);
        }
        
        // Unused.
        else if (materialLine == "illum")
        {
            int dummy;
            fin >> dummy;
        }
    }
    
    fin.close();
    
    return true;
}
    
void Mesh::CreateRenderData()
{
	// Allocate a VBO for each material found.
    for (MeshList::iterator iter = m_meshes.begin(); iter != m_meshes.end(); ++iter)
	{
        gfx::Mesh::MeshPiece *piece = &(iter->second);

        // Generate the primitive for this mesh piece.
        rlGenPrimitives(1, &(piece->primitive));
        rlBindPrimitive(RL_PRIMITIVE, piece->primitive);
        rlPrimitiveParameterString(RL_PRIMITIVE, RL_PRIMITIVE_NAME, piece->material.name.c_str());
        rlBindPrimitive(RL_PRIMITIVE, RL_NULL_PRIMITIVE);

        // Generate any textures that this mesh needs.
        piece->material.diffuseTexture.createFromLoadedData(true);
        piece->material.normalTexture.createFromLoadedData(true);

        size_t numElements = piece->vertices.size();
        
        // Create and load the VBOs.
        piece->buffers[VERTICES].Load(&(piece->vertices[0]), numElements * sizeof(math::Vec3f), "positions");
        piece->buffers[NORMALS].Load(&(piece->normals[0]), numElements * sizeof(math::Vec3f), "normals");
        piece->buffers[TEX_COORDS].Load(&(piece->texCoords[0]), numElements * sizeof(math::Vec2f), "tex coords");
        piece->buffers[TANGENTS].Load(&(piece->tangents[0]), numElements * sizeof(math::Vec3f), "tangents");
        
        piece->numElements = numElements;
	}
}
    
} // namespace gfx
