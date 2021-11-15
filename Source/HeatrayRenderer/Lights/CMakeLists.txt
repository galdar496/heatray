target_sources(HeatrayRenderer PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/DirectionalLight.h
    ${CMAKE_CURRENT_LIST_DIR}/DirectionalLight.cpp
    ${CMAKE_CURRENT_LIST_DIR}/EnvironmentLight.h
    ${CMAKE_CURRENT_LIST_DIR}/EnvironmentLight.cpp
    ${CMAKE_CURRENT_LIST_DIR}/SceneLighting.h
    ${CMAKE_CURRENT_LIST_DIR}/SceneLighting.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ShaderLightingDefines.h
)
