#include "Scene.h"

#include "AssimpMeshProvider.h"
#include "MeshProvider.h"

#include <HeatrayRenderer/Materials/Material.h>
#include <RLWrapper/Program.h>

std::shared_ptr<Scene> Scene::create()
{
	return std::shared_ptr<Scene>(new Scene());
}

void Scene::loadFromDisk(const std::string& path, bool convertToMeters, bool swapYZ)
{
	// We use Assimp to load scene data from disk.
	AssimpMeshProvider provider(path, convertToMeters, swapYZ);
	m_aabb = provider.sceneAABB();

	auto &materials = provider.GetMaterials();
	Mesh mesh(&provider, materials, m_newProgramCreatedCallback, glm::mat4(1.0f));
	bindLighting(mesh);
	
	m_meshes.emplace_back(std::move(mesh));
}

size_t Scene::addMesh(MeshProvider *meshProvider, std::vector<std::shared_ptr<Material>>&& materials, const glm::mat4& transform)
{
	Mesh mesh(meshProvider, std::move(materials), m_newProgramCreatedCallback, transform);
	bindLighting(mesh);
	
	m_meshes.emplace_back(std::move(mesh));
	return (m_meshes.size() - 1);
}

void Scene::bindLighting(const Mesh& mesh)
{
	// Ensure any newly created programs are setup to bind scene lighting data.
	for (auto& submesh : mesh.submeshes()) {
		submesh.primitive->bind();
		m_lighting->bindLightingBuffersToProgram(submesh.material->program());
		submesh.primitive->unbind();
	}
}