//
//  Scene.h
//  Heatray
//
//  Encapsulates an entire scene including: geometry, materials, and lighting.
//
//

#pragma once

#include "Lighting.h"
#include "Mesh.h"

#include <Utility/AABB.h>

#include <Metal/MTLDevice.hpp>

#include <functional>
#include <string_view>
#include <vector>

// Forward declarations.
class Material;
class MeshProvider;

class Scene
{
public:
	static std::shared_ptr<Scene> create();
	~Scene() { clearAll(); }

	//-------------------------------------------------------------------------
	// Load a mesh from disk. It is recommended to use this function instead
	// of the AssimpMeshProvider directly.
	void loadFromDisk(const std::string_view path, bool convertToMeters);

	//-------------------------------------------------------------------------
	// Add a new mesh to the scene via the various supported MeshProviders.
	// Returns an index where this mesh is placed within the internal list of
	// meshes.
	size_t addMesh(MeshProvider* meshProvider, std::vector<std::shared_ptr<Material>> &&materials, const matrix_float4x4& transform, MTL::Device* device);

	//-------------------------------------------------------------------------
	// Erase an already-created mesh based on the index returned from addMesh().
	void removeMesh(size_t meshIndex) { m_meshes.erase(m_meshes.begin() + meshIndex); }

	//-------------------------------------------------------------------------
	// Apply a transform to the scene that will affect all mesh objects.
    void applyTransform(const matrix_float4x4& transform);

    void clearMeshesAndMaterials();
    //void clearLighting() { m_lighting->clear(); }
	void clearAll() {
		clearMeshesAndMaterials();
		//clearLighting();
	}

	//std::shared_ptr<Lighting> lighting() { return m_lighting; }
	const std::vector<Mesh> &meshes() { return m_meshes; }

	const util::AABB &aabb() const { return m_aabb; }

private:
	Scene() {
		//m_lighting = std::shared_ptr<Lighting>(new Lighting);
	}
	void bindLighting(const Mesh &mesh);

	std::vector<Mesh> m_meshes;
	//std::shared_ptr<Lighting> m_lighting = nullptr;

	util::AABB m_aabb; // AABB that encapsulates the scene.
};
