#pragma once

#include "Material.hpp"
#include "OpenGL.hpp"

#include <memory>

class Mesh {

public:
    Mesh() = delete;
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    Mesh(GLuint vbo, GLuint vbi, GLuint ibo_count, std::shared_ptr<Material> material) noexcept;
    virtual ~Mesh() noexcept;

    void draw(
        GLuint diffuse_color_location,
        GLuint material_uniform_location,
        GLint shininess_location
    ) const noexcept;

    std::shared_ptr<Material> getMaterial() const noexcept;

    static GLuint CreateVertexBuffer(const void *const data, GLsizeiptr size) noexcept;

    static GLuint CreateElementBuffer(const void *const data, GLsizeiptr size) noexcept;

private:
    // Vertex and index buffer object IDs
    GLuint m_vbo;
    GLuint m_ibo;

    // Vertex Array Object
    GLuint m_vao;

    GLuint m_ibo_count;

    // Optional GL texture attached to mesh (0 = none)
    std::shared_ptr<Material> m_material;
};
