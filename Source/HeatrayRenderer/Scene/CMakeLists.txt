add_library(Scene STATIC
    AssimpMeshProvider.h
    AssimpMeshProvider.cpp
    MeshProvider.h
    Lighting.h
    Lighting.cpp
    Mesh.h
    Mesh.cpp
    PlaneMeshProvider.h
    Scene.h
    Scene.cpp
    SphereMeshProvider.h
)

target_link_libraries(Scene
    assimp
)
