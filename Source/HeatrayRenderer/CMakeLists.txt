add_library(HeatrayRenderer STATIC
	FlyCamera.h
	OrbitCamera.h
	PassGenerator.h
	PassGenerator.cpp
	HeatrayRenderer.h
	HeatrayRenderer.cpp
	RLMesh.h
	RLMesh.cpp
)

target_link_libraries(HeatrayRenderer
	MeshProviders
	OpenFileDialog
)

add_subdirectory(Materials)
add_subdirectory(MeshProviders)