#pragma once

#include <glad/glad.h>

struct DrawArraysIndirectCommand {
    GLuint count;        // Number of vertices per instance
    GLuint instanceCount;// Number of instances to render
    GLuint first;        // First vertex
    GLuint baseInstance; // First instance

    DrawArraysIndirectCommand() = default;

    DrawArraysIndirectCommand(GLuint _count, GLuint _instance_count, GLuint _first, GLuint _base_instance) : 
        count(_count), instanceCount(_instance_count), first(_first), baseInstance(_base_instance)  {}
};
