//
//  Mesh.h
//  Heatray
//
//  Created by Cody White on 5/12/14.
//
//

#pragma once

#include "Buffer.h"
#include "Material.h"
#include "Triangle.h"
#include "Program.h"
#include <OpenRL/rl.h>
#include <string>
#include <vector>
#include <unordered_map>

namespace gfx
{
    
///
/// Handle the loading and management of triangle mesh objects.
///

class Mesh
{
    public:
    
        Mesh();
        ~Mesh();
    
        ///
		/// Load a mesh and possibly prepare it for rendering (based on parameters passed to the function).
    	/// Each mesh is broken into mesh pieces which are determined based on all of the triangles specified
    	/// for a given material in the mesh file.
        ///
        /// @param filename Path to the mesh obj file to load.
        /// @param createRenderData If true, render data for each mesh piece is automatically created.
        /// @param scale Scaling to apply to the mesh.
        /// @param clearData If true, delete any loaded data after uploading it to a VBO.
        ///
        /// @return If true, the mesh was successfully loaded.
        ///
        bool Load(const std::string &filename, const bool create_render_data = true, const float scale = 1.0f, const bool clear_data = true);
    
        ///
        /// Destroy this mesh. All mesh pieces will be deallocated as well as any materials
        /// that were loaded.
        ///
        void Destroy();
    
        ///
        /// Clear all loaded data not already in a VBO.
        ///
        void ClearLoadedData();
    
        ///
        /// Create rendering data for all of the mesh pieces. This includes the RL primitives for each material along with
        /// the VBOs to store the geometry. Needs to be called on the same thread that the OpenRL context was created on.
        ///
        void CreateRenderData();
    
        ///
    	/// Struct to populate for preparing the mesh for rendering. Set each member of the struct
    	/// to the proper vertex program attribute location for data uploading to a vertex shader.
        ///
    	struct RenderData
        {
            RenderData() :
            	positionAttribute(-1),
            	normalAttribute(-1),
            	texCoordAttribute(-1),
                tangentAttribute(-1)
            {
            }
            
        	RLint positionAttribute;    ///< The location of the position attribute in a vertex program.
            RLint normalAttribute;	    ///< The location of the normal attribute in a vertex program.
            RLint texCoordAttribute;    ///< The location of the tex coord attribute in a vertex program.
            RLint tangentAttribute;     ///< The location of the tangent attribute in a vertex program.
        };                       
    
        // A separate VBO is created for each type of data, defined by this enum.
        enum VBOType
        {
            kVertices = 0,
            kNormals,
            kTexCoords,
            kTangents,
            
            kNumVBOTypes // This should always be last.
        };
    
        ///
        /// Submesh piece. Each piece is created with a material and a list of
        /// triangles that use that material.
        ///
        struct MeshPiece
        {
            MeshPiece() : primitive(RL_NULL_PRIMITIVE), numElements(0) {}
            
            gfx::Material              material;      ///< Material associated with this piece.
            gfx::Program               program;       ///< Material program bound to this primitive.
            RLprimitive                primitive;     ///< RL primitive which will contain this mesh piece.
            std::vector<math::Vec3f>   vertices;      ///< Vertices of this mesh.
            std::vector<math::Vec3f>   normals;       ///< Normals of this mesh.
            std::vector<math::Vec3f>   tangents;      ///< Calculated tangents of this mesh.
            std::vector<math::Vec2f>   texCoords;     ///< Texture coordinates for this mesh.
            size_t                     numElements;   ///< Number of elements (vertices) to render.
            
            gfx::Buffer buffers[kNumVBOTypes]; ///< VBOs which makeup the mesh data for this piece.
        };
    
        typedef std::unordered_map<std::string, MeshPiece>	MeshList;
    
        ///
    	/// Get a reference to the internal mesh list.
        ///
    	MeshList &GetMeshList();
    
        ///
    	/// Get the name of the mesh.
        ///
    	std::string GetName() const;
    
        ///
    	/// Get the scaling value applied to the mesh.
        ///
    	float GetScale() const;
    
    private:
    
    	// This class is not copyable.
    	Mesh(const Mesh &other) = delete;
    	Mesh & operator=(const Mesh &other) = delete;
    
        ///
        /// Load all materials from the mesh.
        ///
        /// @param filename Relative path to the mesh file to parse.
        /// @param basePath Base path shared by the obj mesh file and the material file.
        /// @param clearData If true, clear any loaded data once it's inside of OpenRL.
        /// @param materials All materials popluated after reading the file (OUT).
        ///
        /// If true, the materials were successfully loaded.
        ///
        bool LoadMaterials(const std::string &filename, const std::string &base_path, const bool clear_data, MeshList &materials);
        
        // Member variables.
        MeshList    m_meshes;	   ///< Map of pieces of this mesh.
    	std::string m_meshName;   ///< Name of the mesh (usually the path).
    	float       m_meshScale;  ///< Scaling to apply to the mesh.
};
    
} // namespace gfx




