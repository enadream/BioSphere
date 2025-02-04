#ifndef DRAW_ARRAY_COMMAND_HPP
#define DRAW_ARRAY_COMMAND_HPP

#include <glad/glad.h>
#include <GLFW/glfw3.h>

struct DrawArraysIndirectCommand {
    GLuint count;        // Number of vertices per instance
    GLuint instanceCount;// Number of instances to render
    GLuint first;        // First vertex
    GLuint baseInstance; // First instance

    DrawArraysIndirectCommand() = default;

    DrawArraysIndirectCommand(GLuint _count, GLuint _instance_count, GLuint _first, GLuint _base_instance) : 
        count(_count), instanceCount(_instance_count), first(_first), baseInstance(_base_instance)  {}
};

#endif