#include "SkeletonTree.hpp"

#include <iostream>

#define MAX_BONES 128u

SkeletonTree::SkeletonTree(
    std::shared_ptr<Armature>&& armature,
    GLuint original_buffer,
    GLuint per_frame_buffer
) noexcept :
    m_armature(std::move(armature)),
    m_BonesOriginalBuffer(original_buffer),
    m_BonesPerFrameBuffer(per_frame_buffer),
    m_BonesCount(0),
    m_BonesNameToIndex()
{

};

SkeletonTree::~SkeletonTree() {
    if (m_BonesOriginalBuffer != 0) {
        CHECK_GL_ERROR(glDeleteBuffers(1, &m_BonesOriginalBuffer));
    }

    if (m_BonesPerFrameBuffer != 0) {
        CHECK_GL_ERROR(glDeleteBuffers(1, &m_BonesPerFrameBuffer));
    }
}

SkeletonTree* SkeletonTree::CreateSkeletonTree(
    std::shared_ptr<Armature> armature
) noexcept {
    GLuint original_buffer = 0, per_frame_buffer = 0;

    // Create a shader storage buffer (SSBO) to hold the original skeleton data.
    CHECK_GL_ERROR(glGenBuffers(1, &original_buffer));
    CHECK_GL_ERROR(glBindBuffer(GL_SHADER_STORAGE_BUFFER, original_buffer));
    CHECK_GL_ERROR(glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        static_cast<GLsizeiptr>(MAX_BONES * sizeof(SkeletonGPUElement)),
        nullptr,
        GL_STATIC_DRAW
    ));
    CHECK_GL_ERROR(glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0));

    // Create a shader storage buffer (SSBO) to hold the per-frame skeleton data.
    CHECK_GL_ERROR(glGenBuffers(1, &per_frame_buffer));
    CHECK_GL_ERROR(glBindBuffer(GL_SHADER_STORAGE_BUFFER, per_frame_buffer));
    CHECK_GL_ERROR(glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        static_cast<GLsizeiptr>(MAX_BONES * sizeof(glm::mat4)),
        nullptr,
        GL_DYNAMIC_DRAW
    ));
    CHECK_GL_ERROR(glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0));

    return new SkeletonTree(std::move(armature), original_buffer, per_frame_buffer);
}

void SkeletonTree::bind(GLuint bindingPoint) const noexcept {
    if (m_BonesPerFrameBuffer == 0) return;
    CHECK_GL_ERROR(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, m_BonesPerFrameBuffer));
}

bool SkeletonTree::addBone(
    const ArmatureNodeRef& armature_node_ref,
    const glm::mat4& offset_matrix
) noexcept {
    if (m_BonesCount >= MAX_BONES) {
        std::cerr << "Maximum number of bones reached: " << MAX_BONES << std::endl;
        return false;
    }

    const auto armature_node_index_opt = m_armature->findArmatureNode(armature_node_ref);
    if (!armature_node_index_opt.has_value()) {
        std::cerr << "Failed to find armature node for bone with ref " << armature_node_ref << std::endl;
        return false;
    }

    const SkeletonGPUElement bone_data = {
        .offset_matrix = offset_matrix,
        .armature_node_index = armature_node_index_opt.value(),
    };

    // Update the SSBO with the new bone data.
    {
        CHECK_GL_ERROR(glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_BonesOriginalBuffer));
        CHECK_GL_ERROR(glBufferSubData(
            GL_SHADER_STORAGE_BUFFER,
            static_cast<GLsizeiptr>(m_BonesCount * sizeof(SkeletonGPUElement)),
            sizeof(SkeletonGPUElement),
            &bone_data
        ));
        CHECK_GL_ERROR(glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0));
    }

    // Store the bone name to index mapping.
    m_BonesNameToIndex[armature_node_ref] = m_BonesCount;

    ++m_BonesCount;

    return true;
}

uint32_t SkeletonTree::getBoneCount() const noexcept {
    return m_BonesCount;
}

std::optional<uint32_t> SkeletonTree::getBoneIndex(const ArmatureNodeRef& armature_node_ref) const noexcept {
    auto it = m_BonesNameToIndex.find(armature_node_ref);
    if (it != m_BonesNameToIndex.end()) {
        return it->second;
    }

    return std::nullopt;
}
