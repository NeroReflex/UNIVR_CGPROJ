#pragma once

#include "SkeletonTree.hpp"
#include "Material.hpp"
#include "OpenGL.hpp"

#include <memory>
#include <glm/glm.hpp>

struct VertexData {
    float position_x;
    float position_y;
    float position_z;

    float normal_x;
    float normal_y;
    float normal_z;

    float texcoord_u;
    float texcoord_v;

    uint32_t bone_index_0;
    float bone_weight_0;

    uint32_t bone_index_1;
    float bone_weight_1;

    uint32_t bone_index_2;
    float bone_weight_2;

    uint32_t bone_index_3;
    float bone_weight_3;
};

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
        std::shared_ptr<SkeletonTree> m_skeleton_tree = nullptr,
        const glm::mat4& model = glm::mat4(1.0f)
    ) noexcept;

    virtual ~Mesh() noexcept;

    void draw(
        GLuint diffuse_color_location,
        GLuint material_uniform_location,
        GLint shininess_location,
        GLint skeleton_binding = -1
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

    std::shared_ptr<SkeletonTree> m_skeleton_tree;
};
