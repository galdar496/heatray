add_executable(Heatray
  main.cpp
)

include_directories(
  ../../3rdParty/glm
)

target_link_libraries(Heatray
  imgui
  glm
  assimp
  tinyxml2
  FreeImage
  HeatrayRenderer
  Utility
)
if (WIN32)
  target_link_libraries(Heatray
    freeglut
    glew32
    openrl
  )
  add_custom_command(TARGET Heatray POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${PROJECT_SOURCE_DIR}/3rdParty/GLUT/freeglut.dll"
    "${PROJECT_SOURCE_DIR}/3rdParty/glew/glew32.dll"
	  "${PROJECT_SOURCE_DIR}/3rdParty/OpenRL/OpenRL.dll"
	  "${PROJECT_SOURCE_DIR}/3rdParty/OpenRL/OpenRL_pthread.dll"
	  "${PROJECT_SOURCE_DIR}/3rdParty/OpenRL/CausticGLUT.dll"
	  "${PROJECT_SOURCE_DIR}/3rdParty/FreeImage/FreeImage.dll"
      ${CMAKE_CURRENT_BINARY_DIR}
  )
endif()
if (APPLE)
  set_target_properties(Heatray PROPERTIES
    COMPILE_FLAGS "-F ${PROJECT_SOURCE_DIR}/3rdParty/OpenRL"
    LINK_FLAGS "-F ${PROJECT_SOURCE_DIR}/3rdParty/OpenRL"
  )
  target_link_libraries(Heatray
    "-framework OpenGL"
    "-framework GLUT"
    "-framework OpenRL"
  )
endif()

# Annoying "touchfile" based way of getting resources copied in whenever they
# change.  By having our custom command both copy the resource files AND touch a
# sentinel file while ALSO depending on all the resource files, then any time a
# resource file is touched, it kicks the custom command into being out of date,
# and the corresponding target HeatrayResources will then be out of date because
# of the fact it depends on the touch file.  If we don't have a touchfile then
# the resources will copy EVERY time we build.  When changing a shader, every
# resource will be copied, which will get cumbersome, so we might want to switch
# to multiple resource targets, one for shaders, one for images, etc.
#   CMake docs warn that globbing and "CONFIGURE_DEPENDS" are both
# rather unreliable ways of doing this stuff because CMake can't always generate
# a build that can properly check that CMake itself needs to be run again.  If
# we want to fix this, we should manually list the resource files in the
# CMakeLists.txt.
file(GLOB_RECURSE HEATRAY_RESOURCES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/Resources/*")

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/Resource.touchfile
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${PROJECT_SOURCE_DIR}/Resources"
    ${CMAKE_CURRENT_BINARY_DIR}/Resources
  COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/Resource.touchfile
  DEPENDS ${HEATRAY_RESOURCES}
)

add_custom_target(HeatrayResources DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/Resource.touchfile ${HEATRAY_RESOURCES})

add_dependencies(Heatray
  HeatrayResources
)

add_subdirectory(Utility)
add_subdirectory(RLWrapper)
add_subdirectory(HeatrayRenderer)

