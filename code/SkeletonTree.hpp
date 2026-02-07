#pragma once

#include "Buffer.hpp"
#include "Armature.hpp"

#include <unordered_map>
#include <vector>
#include <string>
#include <optional>
#include <glm/glm.hpp>

#define BONE_IS_ROOT 0xFFFFFFFFu

struct SkeletonGPUElement {
    glm::mat4 offset_matrix;

    uint32_t armature_node_index;

    uint32_t padding_1[3];
};

static_assert(offsetof(SkeletonGPUElement, offset_matrix) == 0, "Wrong offset for offset_matrix in ArmatureGPUElement");
static_assert(offsetof(SkeletonGPUElement, armature_node_index) == 64, "Wrong offset for armature_node_index in ArmatureGPUElement");
static_assert(sizeof(SkeletonGPUElement) == 80, "Wrong size for SkeletonGPUElement");

// indice=boneIndex, elemento=(parentBoneIndex, boneOffsetMatrix)
typedef std::vector<SkeletonGPUElement> Skeleton;

typedef std::unordered_map<ArmatureNodeRef, uint32_t> ArmatureNodeRefToIndexMap;

class SkeletonTree {
public:
    SkeletonTree(
        std::shared_ptr<Armature>&& armature,
        GLuint original_buffer,
        GLuint per_frame_buffer
    ) noexcept;

    ~SkeletonTree();

    SkeletonTree(const SkeletonTree&) = delete;

    SkeletonTree& operator=(const SkeletonTree&) = delete;

    /**
    * Bind the internal SSBO to the given shader storage binding point.
    * 
    * WARNING: callers can pass -1 for "not found"!!!
    */
    void bind(GLint bindingPoint) const noexcept;

    /**
     * Add a bone to the skeleton tree if not already present.
     * 
     * @param armature_node_ref a reference to the bone to find it.
     * @param bone_data The GPU data for the bone.
     * @return bool false IIF the operation failed.
     */
    bool addBone(
        const ArmatureNodeRef& armature_node_ref,
        const glm::mat4& offset_matrix
    ) noexcept;

    uint32_t getBoneCount() const noexcept;

    std::optional<uint32_t> getBoneIndex(const ArmatureNodeRef& armature_node_ref) const noexcept;

    static SkeletonTree* CreateSkeletonTree(
        std::shared_ptr<Armature> armature
    ) noexcept;

    // Get the raw GL buffer id for the per-frame bones SSBO.
    inline GLuint getPerFrameBuffer() const noexcept { return m_BonesPerFrameBuffer; }

    // Get the raw GL buffer id for the original (static) bones SSBO.
    inline GLuint getOriginalBuffer() const noexcept { return m_BonesOriginalBuffer; }

    inline std::shared_ptr<Armature> getArmature(void) const noexcept { return m_armature; }

private:
    std::shared_ptr<Armature> m_armature;

    ArmatureNodeRefToIndexMap m_BonesNameToIndex;

    GLuint m_BonesOriginalBuffer;

    GLuint m_BonesPerFrameBuffer;

    GLuint m_BonesCount;
};
