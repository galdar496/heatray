add_library(Scene STATIC
    AssimpMeshProvider.h
    AssimpMeshProvider.cpp
    MeshProvider.h
    PlaneMeshProvider.h
    SphereMeshProvider.h
)

target_link_libraries(Scene
    assimp
)
