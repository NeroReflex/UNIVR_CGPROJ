#include "Shader.hpp"

#include <cstdio>

static void printShaderLog(GLuint shader) {
    GLint len = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
    if (len > 1) {
        auto log = new char[len];
        glGetShaderInfoLog(shader, len, NULL, log);
        fprintf(stderr, "Shader log:\n%s\n", log);
        delete [] log;
    }
}

Shader::Shader(GLuint shader) noexcept : m_shader(shader) { }

Shader::~Shader() {
    glDeleteShader(m_shader);
}

GLuint Shader::CompileShader(GLenum type, const char *source) noexcept {
    GLuint shader = glCreateShader(type);
    if (!shader) return 0;

    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        fprintf(stderr, "Shader failed to compile.\n");
        printShaderLog(shader);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}
