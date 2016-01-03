//
//  Mesh.h
//  Heatray
//
//  Created by Cody White on 5/12/14.
//
//

#pragma once

#include <OpenRL/rl.h>
#include <string>
#include <vector>
#include <unordered_map>
#include "Buffer.h"
#include "Material.h"
#include "Triangle.h"
#include "Program.h"

namespace gfx
{
    
class Mesh
{
    public:
    
        /// Default constructor.
        Mesh();
    
        /// Destructor.
        ~Mesh();
    
		/// Load a mesh and possibly prepare it for rendering (based on parameters passed to the function).
    	/// Each mesh is broken into mesh pieces which are determined based on all of the triangles specified
    	/// for a given material in the mesh file.
        bool load(const std::string &filename,    // IN: Path to the mesh obj file to load.
                  const bool create_vbo = true,   // IN: If true, VBOs are created for each piece of the mesh.
                  const float scale = 1.0f,		  // IN: Scaling to apply to the mesh.
                  const bool clear_data = true    // IN: If true, delete any loaded data after uploading it to a VBO.
                 );
    
        /// Destroy this mesh.
        void destroy();
    
        /// Clear all loaded data not already in a VBO.
        void clearLoadedData();
    
        /// Create VBOs for all of the mesh pieces.
        void createVBOs();
    
    	/// Struct to populate for preparing the mesh for rendering. Set each member of the struct
    	/// to the proper vertex program attribute location for data uploading to a vertex shader.
    	struct RenderData
        {
            /// Default constructor.
            RenderData() :
            	position_attribute(-1),
            	normal_attribute(-1),
            	tex_coord_attribute(-1),
                tangent_attribute(-1)
            {
            }
            
        	RLint position_attribute;   // The location of the position attribute in a vertex program.
            RLint normal_attribute;	    // The location of the normal attribute in a vertex program.
            RLint tex_coord_attribute;  // The location of the tex coord attribute in a vertex program.
            RLint tangent_attribute;    // The location of the tangent attribute in a vertex program.
        };                       
    
        // A separate VBO is created for each type of data, defined by this enum.
        enum VBOType
        {
            VERTICES = 0,
            NORMALS,
            TEX_COORDS,
            TANGENTS,
            NUM_VBO_TYPES
        };
    
        /// Submesh piece. Each piece is created with a material and a list of
        /// triangles that use that material.
        struct MeshPiece
        {
            MeshPiece() : primitive(RL_NULL_PRIMITIVE), num_elements(0) {}
            
            gfx::Material              material;      // Material associated with this piece.
            gfx::Program               program;       // Material program bound to this primitive.
            RLprimitive                primitive;     // RL primitive which will contain this mesh piece.
            std::vector<math::vec3f>   vertices;      // Vertices of this mesh.
            std::vector<math::vec3f>   normals;       // Normals of this mesh.
            std::vector<math::vec3f>   tangents;      // Calculated tangents of this mesh.
            std::vector<math::vec2f>   tex_coords;    // Texture coordinates for this mesh.
            size_t                      num_elements; // Number of elements (vertices) to render.
            
            gfx::Buffer buffers[NUM_VBO_TYPES]; // VBOs which makeup the mesh data for this piece.
        };
    
        typedef std::unordered_map<std::string, MeshPiece>	MeshList;
    
    	/// Get a reference to the internal mesh list.
    	MeshList &getMeshList();
    
    	/// Get the name of the mesh.
    	std::string getName() const;
    
    	/// Get the scaling value applied to the mesh.
    	float getScale() const;
    
    private:
    
    	// This class is not copyable.
    	Mesh(const Mesh &other);
    	Mesh & operator=(const Mesh &other);
    
        /// Load all materials from the mesh.
        bool loadMaterials(const std::string &filename,   //  IN: Relative path to the mesh file to parse.
                           MeshList &materials,           // OUT: All materials popluated after reading the file.
                           const std::string &base_path,  //  IN: Base path shared by the obj mesh file and the material file.
                           const bool clear_data          //  IN: If true, clear any loaded data once it's inside of OpenRL.
                          );
        
        // Member variables.
        MeshList    m_meshes;	   // Map of pieces of this mesh.
    	std::string m_mesh_name;   // Name of the mesh (usually the path).
    	float       m_mesh_scale;  // Scaling to apply to the mesh.
};
    
} // namespace gfx




