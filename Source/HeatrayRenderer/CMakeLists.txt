add_library(HeatrayRenderer STATIC
    FlyCamera.h
    OrbitCamera.h
    PassGenerator.h
    PassGenerator.cpp
    HeatrayRenderer.h
    HeatrayRenderer.cpp
)

target_link_libraries(HeatrayRenderer
    FileDialog
    Lights
    Materials
    Scene
)

add_subdirectory(Lights)
add_subdirectory(Materials)
add_subdirectory(Scene)
add_subdirectory(Session)