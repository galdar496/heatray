cmake_minimum_required(VERSION 3.18)

project(HeatRay4)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_OSX_ARCHITECTURES x86_64)

function(AddImgui)
    add_subdirectory(3rdParty/imgui)
endfunction(AddImgui)

function(AddGLM)
    add_subdirectory(3rdParty/glm)
endfunction(AddGLM)

function(AddTinyXML2)
    add_subdirectory(3rdParty/tinyxml2)
endfunction(AddTinyXML2)

# Stick assimp into a CMake function so options we set don't pollute our entire project
function(AddAssimp)
  # Build Assimp static
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
  SET(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
  set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
  add_subdirectory(3rdParty/assimp)
endfunction(AddAssimp)

function(AddGLFW)
  # Build GLFW static
  set(BUILD_SHARED_LIBS OFF)
  add_subdirectory(3rdParty/glfw)
endfunction(AddGLFW)

AddImgui()
AddGLM()
AddTinyXML2()
AddAssimp()
AddGLFW()

# Set up include path roots.
include_directories(
  3rdParty/assimp/include/
  3rdParty/glfw/include/
  3rdParty/
  Source_Old
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
add_subdirectory(Source_Old)
add_subdirectory(Resources)
