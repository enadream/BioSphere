# BioSphere

BioSphere is an experimental C++ game project developed in January 2025, focusing on procedural terrain generation using Perlin noise. This project serves as a demonstration of real-time 3D rendering techniques and mathematical problem-solving within graphics programming.

While BioSphere is not a fully playable game, it successfully renders a dynamic 3D terrain composed of spheres, showcasing a custom-built rendering engine.

## Video Demonstration

To see a demonstration of BioSphere in action, click the image below:

[![Watch the video](https://i3.ytimg.com/vi/q9NTqszRE6A/maxresdefault.jpg)](https://youtu.be/q9NTqszRE6A)




## Technologies Used

* C++: Core programming language.

* OpenGL: Graphics API for rendering.

* GLSL: Shader Language for custom rendering effects.

* GLM (OpenGL Mathematics): For linear algebra operations.

* Perlin Noise: For procedural terrain generation.

## Project Highlights

* Custom Rendering Engine: Developed from scratch using C++ and OpenGL.

* Procedural Terrain Generation: Implements Perlin noise to create dynamic 3D terrain.

* Custom Shaders: Utilizes GLSL for advanced visual effects and efficient sphere rendering.

* Real-time Rendering: Demonstrates techniques for rendering complex scenes in real-time.

* Spatial Mathematics: Addresses challenges related to efficient sphere rendering, chunk management, and real-time terrain generation.

## Build Instructions

To build the BioSphere application, follow these general steps:

1. Clone the repository:
```bash
git clone https://github.com/enadream/BioSphere.git
cd BioSphere
```

2. Create a build directory:
```bash
mkdir build
cd build
```

3. Run CMake and build the project:
This step will vary slightly depending on your operating system.

### How to Build (Windows)

To build the BioSphere application on a Windows environment, after following the general steps above, execute the build.bat batch file from within the `build` directory:

```bash
build.bat
```

This script will handle the compilation process and generate the executable.

## Project Goals and Learnings

This project was undertaken to deepen the understanding of computer graphics, linear algebra, and C++ object-oriented design. Through its development, significant improvements were made in:

* OpenGL programming.

* Shader development.

* Spatial mathematics and problem-solving in graphics.

## Project Structure

The project is organized into several key components:

* `main.cpp`: The main entry point of the application, responsible for setting up the GLFW window, initializing OpenGL, and running the main game loop.

* `ChunkHolder` and `Chunk`: These classes manage the procedural generation and storage of the terrain in chunks. The `ChunkHolder` is responsible for creating and managing all the chunks in the world, while the `Chunk` class represents a single segment of the terrain.

* `Camera`: A camera class that handles the view and projection matrices, allowing for navigation through the 3D world. It also includes functionality for frustum culling.

* `ShaderProgram`: A helper class for loading, compiling, and linking GLSL shaders into a shader program.

* `VertexArray`, `VertexBuffer`, `IndexBuffer`: Wrapper classes for OpenGL buffer objects, simplifying the process of creating and managing vertex and index data.

* `Texture`: A class for loading and managing textures.

* Noise Libraries: The project uses the `FastNoiseLite` library for procedural generation.

## Challenges and Learning

This project was a significant learning experience in several areas of computer graphics and C++ development:

* OpenGL and GLSL: I gained a deep understanding of modern OpenGL, including buffer management, shader programming, and the rendering pipeline.

* Linear Algebra and Spatial Mathematics: I applied concepts of linear algebra for transformations, camera movement, and frustum culling. This project helped solidify my understanding of vectors, matrices, and geometric calculations in a practical context.

* C++ Object-Oriented Design: I designed and implemented a modular and extensible rendering engine, with classes for managing different aspects of the application, such as chunks, camera, and shaders.

* Procedural Generation: I learned how to use Perlin noise to generate natural-looking terrain and the challenges of managing large, procedurally generated worlds efficiently.

* Performance Optimization: I implemented techniques like frustum culling to improve rendering performance and learned about the importance of efficient data management when dealing with a large number of objects.

## Current Status

BioSphere is an experimental project primarily focused on rendering and procedural generation. It successfully demonstrates its core technical objectives but is not intended as a fully playable game.
