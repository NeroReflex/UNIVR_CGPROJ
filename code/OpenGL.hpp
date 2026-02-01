#pragma once

/*
#include <GLES3/gl3.h>
#include <GLES3/gl32.h>
*/

#if !defined(ANDROID) && !defined(__ANDROID__)
    #include "glad/glad.h"
#else
    #include <gles3/gl32.h>
#endif

const char* glErrorString(GLenum err) noexcept;

/* Call CHECK_GL_ERROR() after GL calls; it prints file:line and returns nonzero if any error occurred. */
int check_gl_error_impl(const char* file, int line) noexcept;

#define CHECK_GL_ERROR(x) \
    { \
        x; \
        check_gl_error_impl(__FILE__, __LINE__); \
    }
