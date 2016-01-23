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

/// Default constructor.
Mesh::Mesh()
{
}
    
/// Destructor.
Mesh::~Mesh()
{
    destroy();
}
    
/// Load a mesh and possibly prepare it for rendering (based on parameters passed to the function).
/// Each mesh is broken into mesh pieces which are determined based on all of the triangles specified
/// for a given material in the mesh file.
bool Mesh::load(const std::string &filename, const bool create_render_data, const float scale, const bool clear_data)
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
    
    m_mesh_name  = filename;
    m_mesh_scale = scale;
    
    // Add a default material for use if no material file is specified in the obj file.
    {
        Material default_material;
        default_material.name = "default";
        default_material.roughness = 0.0f;
        default_material.transmissive = math::Vec3f::Zero();
        default_material.specular = math::Vec3f::Zero();
        default_material.diffuse = math::Vec3f::Zero();
        default_material.index_of_refraction = 1.0f;
        m_meshes["default"].material = default_material;
    }
    
    bool use_normals    = false;
    bool use_tex_coords = false;
    
    std::vector<math::Vec3f> vertices;
    std::vector<math::Vec3f> normals;
    std::vector<math::Vec2f> tex_coords;
    
    MeshList::iterator current_mesh = m_meshes.find("default");
    
    // Read the input obj file.
    std::string input_line = "";
	while (fin >> input_line)
	{
		// The '#' character denotes a line comment.  Therefore, ignore everything after this character on the same line.
		if (input_line[0] == '#')
		{
			char tmp[200];
			fin.getline(tmp, 200, '\n');
		}
        
		else if (input_line == "mtllib")
		{
			// This is the material file, parse it.
			std::string material_filename;
			fin >> material_filename;
			material_filename = path + material_filename;
			m_meshes.erase("default");  // Remove the default material because it is not needed.
            
			// Load the materials.
			if (!loadMaterials(material_filename, m_meshes, path, clear_data))
            {
                fin.close();
                return false;
            }
		}
        
		// Vertex
		else if (input_line == "v")
		{
			math::Vec3f vertex;
			fin >> vertex[0] >> vertex[1] >> vertex[2];
			vertex *= scale;
            vertices.push_back(vertex);
		}
        
		// Object name
		else if (input_line == "o")
		{
			std::string name;
			fin >> name;
		}
        
		// Vertex normal
		else if (input_line == "vn")
		{
			math::Vec3f normal;
			fin >> normal[0] >> normal[1] >> normal[2];
            normals.push_back(normal);
            use_normals = true;
		}
        
		// Texture coordinate
		else if (input_line == "vt")
		{
			math::Vec2f tex_coord;
			fin >> tex_coord[0] >> tex_coord[1];
            tex_coords.push_back(tex_coord);
            use_tex_coords = true;
		}
        
		// Current material name.
		else if (input_line == "usemtl")
		{
            std::string current_material;
			fin >> current_material;
            current_mesh = m_meshes.find(current_material);
		}
        
		// Smooth
		else if (input_line == "s")
		{
			std::string smooth;
			fin >> smooth;
		}
        
		// Triangle (face)
		else if (input_line == "f")
		{
            math::Vec3i vertex_indices;
            math::Vec3i normal_indices;
            math::Vec3i tex_coord_indices;
            
			std::string line;
			for (int i = 0; i < 3; ++i)
			{
				fin >> line;
				if (use_normals && !use_tex_coords)
				{
					size_t cut = line.find("/");
					line[cut] = ' ';
					line[cut + 1] = ' ';
					sscanf(line.c_str(), "%d %d", &vertex_indices[i], &normal_indices[i]);
				}
				else if (!use_normals && use_tex_coords)
				{
					size_t cut = line.find("/");
					line[cut] = ' ';
					sscanf(line.c_str(), "%d %d", &vertex_indices[i], &tex_coord_indices[i]);
				}
				else if (!use_normals && !use_tex_coords)
				{
					sscanf(line.c_str(), "%d", &vertex_indices[i]);
				}
				else if (use_normals && use_tex_coords)
				{
					size_t cut = line.find("/");
					line[cut] = ' ';
					cut = line.find("/");
					line[cut] = ' ';
					sscanf(line.c_str(), "%d %d %d", &vertex_indices[i], &tex_coord_indices[i], &normal_indices[i]);
				}
			}
            
            gfx::Triangle triangle;
            triangle.vertices[0] = vertices[vertex_indices[0] - 1];
            triangle.vertices[1] = vertices[vertex_indices[1] - 1];
            triangle.vertices[2] = vertices[vertex_indices[2] - 1];
            
			if (use_normals)
			{
                triangle.normals[0] = normals[normal_indices[0] - 1];
                triangle.normals[1] = normals[normal_indices[1] - 1];
                triangle.normals[2] = normals[normal_indices[2] - 1];
			}
            
			if (use_tex_coords)
			{
                triangle.tex_coords[0] = tex_coords[tex_coord_indices[0] - 1];
                triangle.tex_coords[1] = tex_coords[tex_coord_indices[1] - 1];
                triangle.tex_coords[2] = tex_coords[tex_coord_indices[2] - 1];
                
                triangle.calculateTangents();
			}
            
            // Push the triangle information into the mesh data lists.
            for (int jj = 0; jj < 3; ++jj)
            {
                current_mesh->second.vertices.push_back(triangle.vertices[jj]);
                current_mesh->second.normals.push_back(triangle.normals[jj]);
                current_mesh->second.tangents.push_back(triangle.tangents[jj]);
                current_mesh->second.tex_coords.push_back(triangle.tex_coords[jj]);
            }
            
		}
	}
    
	fin.close();
    
	if (create_render_data)
	{
		createRenderData();
	}
    
	if (clear_data)
	{
		clearLoadedData();
	}
    
    return true;
}
    
/// Destroy this mesh.
void Mesh::destroy()
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
            piece->buffers[ii].destroy();
        }
        
        piece->program.destroy();
        piece->material.destroy();
    }
    
    clearLoadedData();
}
    
/// Clear all loaded data not already in a VBO.
void Mesh::clearLoadedData()
{
    for (MeshList::iterator iter = m_meshes.begin(); iter != m_meshes.end(); ++iter)
    {
		iter->second.vertices.clear();
        iter->second.normals.clear();
        iter->second.tex_coords.clear();
        iter->second.tangents.clear();
    }
}
    
/// Get a reference to the internal mesh list.
Mesh::MeshList &Mesh::getMeshList()
{
    return m_meshes;
}
    
/// Get the name of the mesh.
std::string Mesh::getName() const
{
	return m_mesh_name;
}

/// Get the scaling value applied to the mesh.
float Mesh::getScale() const
{
    return m_mesh_scale;
}
    
/// Load a specific material from the mesh file.
bool Mesh::loadMaterials(const std::string &filename, MeshList &materials, const std::string &base_path, const bool clear_data)
{
    std::ifstream fin;
    fin.open(filename);
    if (!fin)
    {
        std::cout << "Mesh::loadMaterials() - Could not open material file: " << filename << std::endl;
        return false;
    }
    
    std::string material_name = "";
    std::string material_line = "";
    MeshList::iterator material_iter;
    
    // Read the material file.
    while (fin >> material_line)
    {
        if (material_line[0] == '#')
        {
            char tmp[200];
            fin.getline(tmp, 200, '\n');
        }
        
        else if (material_line == "newmtl")
        {
            fin >> material_name;
            materials[material_name].material.name = material_name;
            material_iter = materials.find(material_name);
            if (material_iter == materials.end())
            {
                std::cout << "Mesh::loadMaterials() - Could not insert material " << material_name << " into the material map." << std::endl;
                fin.close();
                return false;
            }

            if (material_name.find("Light") != std::string::npos)
            {
                // This material is going to be used as a light source.
                material_iter->second.material.component_flags.set(gfx::Material::LIGHT);
            }
        }
        
        else if (material_line == "Ns")
        {
            fin >> material_iter->second.material.roughness;
        }
        
        // We'll read Ka as the transmissive component, not the ambient.
        else if (material_line == "Ka")
        {
            fin >> material_iter->second.material.transmissive[0] >> material_iter->second.material.transmissive[1] >> material_iter->second.material.transmissive[2];
            if (material_iter->second.material.transmissive != math::Vec3f::Zero())
            {
                material_iter->second.material.component_flags.set(gfx::Material::TRANSMISSIVE);
            }
        }
        
        else if (material_line == "Kd")
        {
            fin >> material_iter->second.material.diffuse[0] >> material_iter->second.material.diffuse[1] >> material_iter->second.material.diffuse[2];
            if (material_iter->second.material.diffuse != math::Vec3f::Zero())
            {
                material_iter->second.material.component_flags.set(gfx::Material::DIFFUSE);
            }
        }
        
        else if (material_line == "Ks")
        {
            fin >> material_iter->second.material.specular[0] >> material_iter->second.material.specular[1] >> material_iter->second.material.specular[2];
            if (material_iter->second.material.specular != math::Vec3f::Zero())
            {
                material_iter->second.material.component_flags.set(gfx::Material::SPECULAR);
            }
        }
        
        else if (material_line == "Ksub")
        {
            fin >> material_iter->second.material.diffuse[0] >> material_iter->second.material.diffuse[1] >> material_iter->second.material.diffuse[2];
            if (material_iter->second.material.diffuse != math::Vec3f::Zero())
            {
                material_iter->second.material.component_flags.set(gfx::Material::SUBSURFACE);
                material_iter->second.material.component_flags.set(gfx::Material::DIFFUSE);
            }
        }
        
        else if (material_line == "Ni")
        {
            fin >> material_iter->second.material.index_of_refraction;
        }
        
        // Unused.
        else if (material_line == "d")
        {
            int dummy;
            fin >> dummy;
        }
        
        else if (material_line == "map_Kd")
        {
            std::string texture_path;
            fin >> texture_path;
            
            texture_path = base_path + texture_path;
            material_iter->second.material.diffuse_texture.loadTextureData(texture_path);
            material_iter->second.material.component_flags.set(gfx::Material::DIFFUSE_TEXTURE);
        }
        
        else if (material_line == "map_Bump")
        {
            std::string texture_path;
            fin >> texture_path;
            
            texture_path = base_path + texture_path;
            material_iter->second.material.normal_texture.loadTextureData(texture_path);
            material_iter->second.material.component_flags.set(gfx::Material::NORMALMAP);
        }
        
        // Unused.
        else if (material_line == "illum")
        {
            int dummy;
            fin >> dummy;
        }
    }
    
    fin.close();
    
    return true;
}
    
/// Create rendering data for all of the mesh pieces. This includes the RL primitives for each material along with
/// the VBOs to store the geometry. Needs to be called on the same thread that the OpenRL context was created on.
void Mesh::createRenderData()
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
        piece->material.diffuse_texture.createFromLoadedData(true);
        piece->material.normal_texture.createFromLoadedData(true);

        size_t num_elements = piece->vertices.size();
        
        // Create and load the VBOs.
        piece->buffers[VERTICES].load(&(piece->vertices[0]), num_elements * sizeof(math::Vec3f), "positions");
        piece->buffers[NORMALS].load(&(piece->normals[0]), num_elements * sizeof(math::Vec3f), "normals");
        piece->buffers[TEX_COORDS].load(&(piece->tex_coords[0]), num_elements * sizeof(math::Vec2f), "tex coords");
        piece->buffers[TANGENTS].load(&(piece->tangents[0]), num_elements * sizeof(math::Vec3f), "tangents");
        
        piece->num_elements = num_elements;
	}
}
    
} // namespace gfx
