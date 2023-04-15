#include "Scene.h"

#include "AssimpMeshProvider.h"
#include "MeshProvider.h"

#include <HeatrayRenderer/Materials/Material.h>

std::shared_ptr<Scene> Scene::create()
{
	return std::shared_ptr<Scene>(new Scene());
}

void Scene::loadFromDisk(const std::string_view path, bool convertToMeters)
{
	//m_lighting->clearAllButEnvironment();

	// We use Assimp to load scene data from disk.
	//AssimpMeshProvider provider(path, convertToMeters, m_lighting);
	//m_aabb = provider.sceneAABB();

	//auto &materials = provider.GetMaterials();
	//Mesh mesh(&provider, materials, m_newProgramCreatedCallback, matrix_identity_double4x4);
	//bindLighting(mesh);
	
	//m_meshes.emplace_back(std::move(mesh));
}

size_t Scene::addMesh(MeshProvider* meshProvider, std::vector<std::shared_ptr<Material>>&& materials, const matrix_float4x4& transform, MTL::Device* device)
{
	Mesh mesh(meshProvider, materials, transform, device);
	bindLighting(mesh);
	
	m_meshes.emplace_back(std::move(mesh));
	return (m_meshes.size() - 1);
}

void Scene::applyTransform(const matrix_float4x4& transform)
{
	for (auto &mesh : m_meshes) {
		for (auto &submesh : mesh.submeshes()) {
			//submesh.primitive->bind();
			//int worldFromEntityLocation = submesh.material->program()->getUniformLocation("worldFromEntity");
            matrix_float4x4 newTransform = transform * submesh.localTransform;
			//submesh.material->program()->setMatrix4fv(worldFromEntityLocation, &(newTransform[0][0]));
			//submesh.primitive->unbind();
		}
	}
}

void Scene::bindLighting(const Mesh& mesh)
{
	// Ensure any newly created programs are setup to bind scene lighting data.
	for (auto& submesh : mesh.submeshes()) {
		//submesh.primitive->bind();
		//m_lighting->bindLightingBuffersToProgram(submesh.material->program());
		//submesh.primitive->unbind();
	}
}

void Scene::clearMeshesAndMaterials() {
    m_meshes.clear();
}
