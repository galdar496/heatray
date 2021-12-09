#include "Scene.h"

#include "AssimpMeshProvider.h"
#include "MeshProvider.h"

#include <HeatrayRenderer/Materials/Material.h>
#include <RLWrapper/Program.h>

#include <glm/glm/gtx/euler_angles.hpp>

std::shared_ptr<Scene> Scene::create()
{
	return std::shared_ptr<Scene>(new Scene());
}

void Scene::loadFromDisk(const std::string& path, bool convertToMeters)
{
	m_lighting->clearAllButEnvironment();

	// We use Assimp to load scene data from disk.
	AssimpMeshProvider provider(path, convertToMeters, m_lighting);
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

void Scene::applyRotation(const float yaw, const float pitch, const float roll)
{
	glm::mat4 rotation = glm::yawPitchRoll(yaw, pitch, roll);
	for (auto &mesh : m_meshes) {
		for (auto &submesh : mesh.submeshes()) {
			submesh.primitive->bind();
			int worldFromEntityLocation = submesh.material->program()->getUniformLocation("worldFromEntity");
			glm::mat4 transform = rotation * submesh.transform;
			submesh.material->program()->setMatrix4fv(worldFromEntityLocation, &(transform[0][0]));
			submesh.primitive->unbind();
		}
	}

	// TODO: consider how to add lighting here.
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