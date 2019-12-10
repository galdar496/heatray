cmake_minimum_required(VERSION 3.1)

project(HeatRay4)

set(CMAKE_CXX_STANDARD 17)

# Stick assimp into a CMake function so options we set don't pollute our entire project
function(AddAssimp)
  # Build Assimp static
  set(BUILD_SHARED_LIBS OFF)
  add_subdirectory(3rdParty/assimp)
endfunction(AddAssimp)

AddAssimp()

add_subdirectory(3rdParty/imgui)
add_subdirectory(3rdParty/glm)

# Set up include path roots.
include_directories(
  3rdParty/assimp/include/
  3rdParty/
  Source
)

# Set up .lib file search paths for windows.
link_directories(
  3rdParty/FreeImage
  3rdParty/glew
  3rdParty/GLUT
  3rdParty/OpenRL
)

if (APPLE)
  add_definitions(-DGL_SILENCE_DEPRECATION)
endif()

# This causes CMake to look for a CMakeLists.txt in that folder.
add_subdirectory(Source)
