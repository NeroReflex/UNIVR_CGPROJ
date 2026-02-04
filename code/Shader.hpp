#pragma once

#include "OpenGL.hpp"

class Program;

class Shader {

    friend class Program;

public:
    Shader() = delete;
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(GLuint shader) noexcept;

    virtual ~Shader();

protected:
    // Compile an individual shader. Returns shader object or 0 on failure.
    static GLuint CompileShader(GLenum type, const char *source) noexcept;

private:
    GLuint m_shader;
};

class VertexShader : public Shader {
public:
    VertexShader() = delete;
    VertexShader(const VertexShader&) = delete;
    VertexShader& operator=(const VertexShader&) = delete;

    inline VertexShader(GLuint shader) noexcept : Shader(shader) {};

    inline static VertexShader* CompileShader(const char *source) noexcept {
        const auto shader = Shader::CompileShader(GL_VERTEX_SHADER, source);
        return (shader != 0) ? new VertexShader(shader) : nullptr;
    }
};

class GeometryShader : public Shader {
public:
    GeometryShader() = delete;
    GeometryShader(const GeometryShader&) = delete;
    GeometryShader& operator=(const GeometryShader&) = delete;

    inline GeometryShader(GLuint shader) noexcept : Shader(shader) {};

    inline static GeometryShader* CompileShader(const char *source) noexcept {
        const auto shader = Shader::CompileShader(GL_GEOMETRY_SHADER, source);
        return (shader != 0) ? new GeometryShader(shader) : nullptr;
    }
};

class FragmentShader : public Shader {
public:
    FragmentShader() = delete;
    FragmentShader(const FragmentShader&) = delete;
    FragmentShader& operator=(const FragmentShader&) = delete;

    inline FragmentShader(GLuint shader) noexcept : Shader(shader) {};

    inline static FragmentShader* CompileShader(const char *source) noexcept {
        const auto shader = Shader::CompileShader(GL_FRAGMENT_SHADER, source);
        return (shader != 0) ? new FragmentShader(shader) : nullptr;
    }
};

class ComputeShader : public Shader {
public:
    ComputeShader() = delete;
    ComputeShader(const ComputeShader&) = delete;
    ComputeShader& operator=(const ComputeShader&) = delete;

    inline ComputeShader(GLuint shader) noexcept : Shader(shader) {};

    inline static ComputeShader* CompileShader(const char *source) noexcept {
        const auto shader = Shader::CompileShader(GL_COMPUTE_SHADER, source);
        return (shader != 0) ? new ComputeShader(shader) : nullptr;
    }
};
