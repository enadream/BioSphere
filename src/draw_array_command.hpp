#ifndef DRAW_ARRAY_COMMAND_HPP
#define DRAW_ARRAY_COMMAND_HPP

#include <glad/glad.h>
#include <GLFW/glfw3.h>

struct DrawArraysIndirectCommand {
    GLuint count;        // Number of vertices per instance
    GLuint instanceCount;// Number of instances to render
    GLuint first;        // First vertex
    GLuint baseInstance; // First instance
};

#endif