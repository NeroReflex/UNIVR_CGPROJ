#include "Mesh.hpp"

#include <cassert>

Mesh::Mesh(GLuint vbo, GLuint vbi, GLuint ibo_count, std::shared_ptr<Material> material) noexcept
    : m_vao(0), m_vbo(vbo), m_ibo(vbi), m_ibo_count(ibo_count), m_material(material)
{
    // Create and configure a VAO that captures the vertex attribute setup
    glGenVertexArrays(1, &m_vao);

    assert(m_vao != 0 && "Failed to generate vao");

    glBindVertexArray(m_vao);

    // Bind the vertex and index buffers to set up attribute pointers
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);

    // Always use vertex layout: vec3 position (location=0), vec3 normal (location=2), vec2 texcoord (location=1)
    constexpr GLsizei stride = 8 * sizeof(float);
    // position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (const void*)0);
    // normal
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (const void*)(3 * sizeof(float)));
    // texcoord
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (const void*)(6 * sizeof(float)));

    // Unbind VAO to avoid accidental modifications
    glBindVertexArray(0);
}

std::shared_ptr<Material> Mesh::getMaterial() const noexcept {
    return m_material;
}

Mesh::~Mesh() noexcept {
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    if (m_vbo) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_ibo) {
        glDeleteBuffers(1, &m_ibo);
        m_ibo = 0;
    }
}

void Mesh::draw(
    GLuint diffuse_color_location,
    GLuint material_uniform_location
) const noexcept {
    // bind texture to unit 0 if using texture
    getMaterial()->bindRenderState(diffuse_color_location, material_uniform_location);

    // Bind the VAO which already has the vertex attribute state
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_ibo_count, GL_UNSIGNED_SHORT, nullptr);
    glBindVertexArray(0);
}

GLuint Mesh::CreateVertexBuffer(const void *const data, GLsizeiptr size) noexcept {
    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    
    assert(vbo != 0 && "Failed to generate vbo buffer");

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    return vbo;
}

GLuint Mesh::CreateElementBuffer(const void *const data, GLsizeiptr size) noexcept {
    GLuint ibo = 0;
    glGenBuffers(1, &ibo);

    assert(ibo != 0 && "Failed to generate ibo buffer");

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    return ibo;
}
