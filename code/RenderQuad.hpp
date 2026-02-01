#pragma once

#include <memory>
#include "OpenGL.hpp"

class Program;

class RenderQuad {
public:
    RenderQuad() = delete;
    RenderQuad(const RenderQuad&) = delete;
    RenderQuad& operator=(const RenderQuad&) = delete;

    ~RenderQuad();

    // Create a RenderQuad. Returns nullptr on allocation failure.
    static RenderQuad* Create() noexcept;

    // Draw the quad assuming the currently bound program and textures are set.
    void draw() const noexcept;

    // Convenience: bind `texture` to `unit`, set `uniformName` in `program` to that unit, then draw.
    void drawWithTexture(const Program& program, const char* uniformName, GLuint texture, GLuint unit = 0) const noexcept;

    inline GLuint getVAO() const noexcept { return m_vao; }
    inline GLuint getVBO() const noexcept { return m_vbo; }

private:
    RenderQuad(GLuint vao, GLuint vbo) noexcept;

    GLuint m_vao;
    GLuint m_vbo;
};
