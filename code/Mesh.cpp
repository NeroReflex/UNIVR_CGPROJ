#include "Mesh.hpp"

#include <cassert>

Mesh::Mesh(
    GLuint vbo,
    GLuint vbi,
    GLuint ibo_count,
    std::shared_ptr<Material> material,
    std::shared_ptr<SkeletonTree> m_skeleton_tree,
    const glm::mat4& model
) noexcept :
    m_vao(0),
    m_vbo(vbo),
    m_ibo(vbi),
    m_ibo_count(ibo_count),
    m_material(material),
    m_model_matrix(model),
    m_skeleton_tree(m_skeleton_tree)
{
    // Create and configure a VAO that captures the vertex attribute setup
    CHECK_GL_ERROR(glGenVertexArrays(1, &m_vao));

    assert(m_vao != 0 && "Failed to generate vao");

    CHECK_GL_ERROR(glBindVertexArray(m_vao));

    // Bind the vertex and index buffers to set up attribute pointers
    CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, m_vbo));
    CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo));

    // Always use vertex layout: vec3 position (location=0), vec3 normal (location=2), vec2 texcoord (location=1)
    constexpr GLsizei stride = sizeof(VertexData);

    // position
    CHECK_GL_ERROR(glEnableVertexAttribArray(0));
    CHECK_GL_ERROR(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (const void*)((uintptr_t)(offsetof(VertexData, position_x)))));

    // normal
    CHECK_GL_ERROR(glEnableVertexAttribArray(2));
    CHECK_GL_ERROR(glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (const void*)((uintptr_t)(offsetof(VertexData, normal_x)))));

    // texcoord
    CHECK_GL_ERROR(glEnableVertexAttribArray(1));
    CHECK_GL_ERROR(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (const void*)((uintptr_t)(offsetof(VertexData, texcoord_u)))));

    // bone_index_0 and bone_weight_0
    CHECK_GL_ERROR(glEnableVertexAttribArray(3));
    CHECK_GL_ERROR(glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT, stride, (const void*)((uintptr_t)(offsetof(VertexData, bone_index_0)))));
    CHECK_GL_ERROR(glEnableVertexAttribArray(4));
    CHECK_GL_ERROR(glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride, (const void*)((uintptr_t)(offsetof(VertexData, bone_weight_0)))));

    // bone_index_1 and bone_weight_1
    CHECK_GL_ERROR(glEnableVertexAttribArray(5));
    CHECK_GL_ERROR(glVertexAttribIPointer(5, 1, GL_UNSIGNED_INT, stride, (const void*)((uintptr_t)(offsetof(VertexData, bone_index_1)))));
    CHECK_GL_ERROR(glEnableVertexAttribArray(6));
    CHECK_GL_ERROR(glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, stride, (const void*)((uintptr_t)(offsetof(VertexData, bone_weight_1)))));

    // bone_index_2 and bone_weight_2
    CHECK_GL_ERROR(glEnableVertexAttribArray(7));
    CHECK_GL_ERROR(glVertexAttribIPointer(7, 1, GL_UNSIGNED_INT, stride, (const void*)((uintptr_t)(offsetof(VertexData, bone_index_2)))));
    CHECK_GL_ERROR(glEnableVertexAttribArray(8));
    CHECK_GL_ERROR(glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE, stride, (const void*)((uintptr_t)(offsetof(VertexData, bone_weight_2)))));

    // bone_index_3 and bone_weight_3
    CHECK_GL_ERROR(glEnableVertexAttribArray(9));
    CHECK_GL_ERROR(glVertexAttribIPointer(9, 1, GL_UNSIGNED_INT, stride, (const void*)((uintptr_t)(offsetof(VertexData, bone_index_3)))));
    CHECK_GL_ERROR(glEnableVertexAttribArray(10));
    CHECK_GL_ERROR(glVertexAttribPointer(10, 1, GL_FLOAT, GL_FALSE, stride, (const void*)((uintptr_t)(offsetof(VertexData, bone_weight_3)))));

    // Unbind VAO to avoid accidental modifications
    CHECK_GL_ERROR(glBindVertexArray(0));
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
    GLuint specular_color_location,
    GLuint material_uniform_location,
    GLint shininess_location,
    GLint skeleton_binding
) const noexcept {
    // bind texture to unit 0 if using texture and upload material state
    getMaterial()->bindRenderState(diffuse_color_location, specular_color_location, material_uniform_location, shininess_location);

    // Bind the VAO which already has the vertex attribute state
    // If this mesh has a skeleton, bind its SSBO to binding point 0 so shaders can access it.
    if (m_skeleton_tree && skeleton_binding >= 0) {
        assert(skeleton_binding >= 0 && "Missing skeleton binding point for Mesh draw with skeleton");
        m_skeleton_tree->bind(static_cast<GLuint>(skeleton_binding));
    }

    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_ibo_count, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    // Unbind the SSBO from the binding point to avoid accidental reuse.
    if (m_skeleton_tree && skeleton_binding >= 0) {
        CHECK_GL_ERROR(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<GLuint>(skeleton_binding), 0));
    }
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
