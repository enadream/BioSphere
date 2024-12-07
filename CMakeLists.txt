# Minimum Cmake version
cmake_minimum_required(VERSION 3.31)

# Project information
project(BioSphereEvolution VERSION 0.01 LANGUAGES CXX)

# Definitions
add_definitions(-DPROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

# C++ version
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Specify compilers
set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR})

# GLFW
set(GLFW_DIR ${CMAKE_SOURCE_DIR}/deps/glfw-3.4)
option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)
option(GLFW_DOCUMENT_INTERNALS "Include internals in documentation" OFF)
add_subdirectory(${GLFW_DIR} glfw_bin EXCLUDE_FROM_ALL)
include_directories(${GLFW_DIR}/include)

# GLEW
set(GLEW_DIR ${CMAKE_SOURCE_DIR}/deps/glew-2.2.0)
include_directories(${GLEW_DIR}/include)
link_directories(${GLEW_DIR}/build/lib)
add_compile_definitions(-DGLEW_STATIC)

# Libraries
set(LIBRARIES glfw OpenGL::GL libglew32.a opengl32)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Source files
file(GLOB_RECURSE SOURCES src/*.cpp)

# Create main executable
add_executable(BioSphereEvolution ${SOURCES})

# Link glfw and OpenGL library to the executable
target_link_libraries(BioSphereEvolution ${LIBRARIES})