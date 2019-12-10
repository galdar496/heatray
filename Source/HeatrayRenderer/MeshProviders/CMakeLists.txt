add_library(MeshProviders STATIC
	AssimpMeshProvider.h
	AssimpMeshProvider.cpp
	MeshProvider.h
	PlaneMeshProvider.h
	SphereMeshProvider.h
)

target_link_libraries(MeshProviders
	assimp
)
