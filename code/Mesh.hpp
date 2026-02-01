#pragma once

#include "Material.hpp"
#include "OpenGL.hpp"

#include <memory>
#include <glm/glm.hpp>

class Mesh {

public:
    Mesh() = delete;
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    Mesh(
        GLuint vbo,
        GLuint vbi,
        GLuint ibo_count,
        std::shared_ptr<Material> material,
        const glm::mat4& model = glm::mat4(1.0f)
    ) noexcept;

    virtual ~Mesh() noexcept;

    void draw(
        GLuint diffuse_color_location,
        GLuint material_uniform_location,
        GLint shininess_location
    ) const noexcept;

    std::shared_ptr<Material> getMaterial() const noexcept;

    inline const glm::mat4& getModelMatrix() const noexcept { return m_model_matrix; }

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

    // Per-mesh model matrix
    glm::mat4 m_model_matrix;
};
