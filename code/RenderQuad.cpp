#include "RenderQuad.hpp"
#include "OpenGL.hpp"
#include "Program.hpp"

#include <cassert>
#include <cstring>

RenderQuad::RenderQuad(GLuint vao, GLuint vbo) noexcept : m_vao(vao), m_vbo(vbo) {}

RenderQuad::~RenderQuad() {
    if (m_vbo) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
}

RenderQuad* RenderQuad::Create() noexcept {
    GLuint vao = 0, vbo = 0;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    if (vao == 0 || vbo == 0) {
        if (vbo) { glDeleteBuffers(1, &vbo); }
        if (vao) { glDeleteVertexArrays(1, &vao); }
        return nullptr;
    }

    const float quadVertices[] = {
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
    };

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // position (vec2)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (const void*)0);
    // texcoord (vec2)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (const void*)(2 * sizeof(float)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return new RenderQuad(vao, vbo);
}

void RenderQuad::draw() const noexcept {
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void RenderQuad::drawWithTexture(const Program& program, const char* uniformName, GLuint texture, GLuint unit) const noexcept {
    // bind the program and set the sampler uniform
    program.bind();
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture);
    GLint loc = glGetUniformLocation(program.getProgram(), uniformName);
    if (loc >= 0) glUniform1i(loc, static_cast<GLint>(unit));

    draw();
}
