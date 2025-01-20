#include <stdio.h>

static void GlClearError(){
    while (glGetError() != GL_NO_ERROR);
}
static void GlCheckError(const char * message = ""){
    while (GLenum error = glGetError()){
        printf("[OPENGL ERROR]: %u. Checkpoint: %s\n", error, message);
    }
}