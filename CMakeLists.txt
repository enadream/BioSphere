# Minimum Cmake version
cmake_minimum_required(VERSION 3.31)

# Project information
project(BioSphereEvolution VERSION 0.01 LANGUAGES CXX C)

# Definitions
add_definitions(-DPROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

# C++ version
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_CXX_FLAGS "-Wall -std=c++17 -O3") # -O3

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR})

# GLFW
set(GLFW_DIR ${CMAKE_SOURCE_DIR}/deps/glfw-3.4)
# option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
# option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
# option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
# option(GLFW_INSTALL "Generate installation target" OFF)
# option(GLFW_DOCUMENT_INTERNALS "Include internals in documentation" OFF)
# add_subdirectory(${GLFW_DIR} glfw_bin EXCLUDE_FROM_ALL)
include_directories(${GLFW_DIR}/include)
link_directories(${GLFW_DIR}/lib)

# GLAD
set(GLAD_DIR ${CMAKE_SOURCE_DIR}/deps/glad-4.6)
include_directories(${GLAD_DIR}/include)
set(GLAD_SRC ${GLAD_DIR}/src/glad.c)

# STB
include_directories(${CMAKE_SOURCE_DIR}/deps/stb)

# GLMath
include_directories(${CMAKE_SOURCE_DIR}/deps/glm-1.0.2)

# Assimp
set(ASSIMP_DIR ${CMAKE_SOURCE_DIR}/deps/assimp-5.4.3)
include_directories(${ASSIMP_DIR}/include)
include_directories(${ASSIMP_DIR}/build/include)
link_directories(${ASSIMP_DIR}/build/lib)

# Libraries glfw libglew32.a libcglm.dll.a
set(LIBRARIES libglfw3.a OpenGL::GL opengl32 libassimp.dll.a)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Source files
file(GLOB_RECURSE SOURCES src/*.cpp)

# Create main executable
add_executable(BioSphereEvolution ${SOURCES} ${GLAD_SRC})

# Link glfw and OpenGL library to the executable
target_link_libraries(BioSphereEvolution ${LIBRARIES})