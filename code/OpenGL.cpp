#include "OpenGL.hpp"

#include <cstdio>

const char* glErrorString(GLenum err) noexcept {
    switch (err) {
        case GL_NO_ERROR:                      return "GL_NO_ERROR";
        case GL_INVALID_ENUM:                  return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE:                 return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION:             return "GL_INVALID_OPERATION";
        case GL_OUT_OF_MEMORY:                 return "GL_OUT_OF_MEMORY";
        case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
        default: return "Unknown GL error";
    }
}

int check_gl_error_impl(const char* file, int line) noexcept {
    int hadError = 0;
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        fprintf(stderr, "OpenGL ES error %s (0x%04X) at %s:%d\n", glErrorString(err), err, file, line);
        hadError = 1;
    }
    return hadError;
}
