cmake_minimum_required(VERSION 3.1)

project(HeatRay4)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(3rdParty/imgui)
add_subdirectory(3rdParty/glm)
add_subdirectory(3rdParty/tinyxml2)

# Set up include path roots.
include_directories(
  3rdParty/assimp/include/
  3rdParty/glfw/include/
  3rdParty/
  Source
)

# Set up .lib file search paths for windows.
link_directories(
  3rdParty/FreeImage
  3rdParty/glew
  3rdParty/OpenRL
  3rdParty/tinyxml2
)

if (APPLE)
  add_definitions(-DGL_SILENCE_DEPRECATION)
endif()

# This causes CMake to look for a CMakeLists.txt in that folder.
add_subdirectory(Source)
add_subdirectory(Resources)

# Stick assimp into a CMake function so options we set don't pollute our entire project
function(AddAssimp)
  # Build Assimp static
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
  SET(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
  add_subdirectory(3rdParty/assimp)
endfunction(AddAssimp)

function(AddGLFW)
  # Build GLFW static
  set(BUILD_SHARED_LIBS OFF)
  add_subdirectory(3rdParty/glfw)
endfunction(AddGLFW)

AddAssimp()
AddGLFW()