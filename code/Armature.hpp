#pragma once

#include <string>
#include <memory>
#include <optional>
#include <type_traits>

#include <unordered_map>
#include <vector>
#include <string>
#include <optional>
#include <glm/glm.hpp>

#include "Buffer.hpp"

class SkeletonTree;

#define NODE_IS_NOT_BONE -1

struct ArmatureGPUElement {
    glm::mat4 transform;

    // 0 means root/no parent
    uint32_t parent_index;

    uint32_t padding_1[3];
};

static_assert(offsetof(ArmatureGPUElement, transform) == 0, "Wrong offset for transform in ArmatureGPUElement");
static_assert(offsetof(ArmatureGPUElement, parent_index) == 64, "Wrong offset for parent_index in ArmatureGPUElement");
static_assert(sizeof(ArmatureGPUElement) == 80, "Wrong size for ArmatureGPUElement");

typedef uintptr_t ArmatureNodeRef;

typedef std::unordered_map<ArmatureNodeRef, uint32_t> ArmatureNodeRefToIndexMap;

typedef std::unordered_map<std::string, uint32_t> ArmatureNodeNameToIndexMap;

class ArmatureNode {

    friend class Armature;

public:
    ArmatureNode(
        std::weak_ptr<ArmatureNode> parent,
        glm::mat4 transform,
        const std::string& name,
        ArmatureNodeRef node_ref
    ) noexcept;

    ~ArmatureNode() = default;

    void addChild(std::shared_ptr<ArmatureNode> child) noexcept;

    ArmatureNode* findArmatureNodeByNameRecursive(
        const std::string& name
    );

    uint32_t write_armature_data(
        ArmatureNodeRefToIndexMap& armature_node_to_index_map,
        ArmatureNodeNameToIndexMap& armature_node_name_to_index_map,
        std::unordered_map<uintptr_t, uint32_t>& armature_node_to_index,
        GLuint buffer,
        uint32_t parent_index,
        uint32_t& nodes_count
    );

    uint32_t countNodes(void) const noexcept;

    uint32_t countNodesWithValidBones(void) const noexcept;

private:
    std::string m_Name;

    ArmatureNodeRef m_NodeRef;

    glm::mat4 m_transform;

    std::weak_ptr<ArmatureNode> m_Parent;

    std::vector<std::shared_ptr<ArmatureNode>> m_Children;
};

class Armature {
public:
    Armature(
        ArmatureNodeRefToIndexMap&& nodes_ref_to_index,
        ArmatureNodeNameToIndexMap&& nodes_name_to_index,
        GLuint nodes_buffer,
        GLuint nodes_count
    ) noexcept;

    ~Armature() = default;

    Armature(const Armature&) = delete;

    Armature& operator=(const Armature&) = delete;

    static Armature* CreateArmature(
        std::shared_ptr<ArmatureNode> root_node
    ) noexcept;

    std::optional<uint32_t> findArmatureNode(const ArmatureNodeRef& node_ref) const noexcept;

    std::optional<uint32_t> findArmatureNodeByName(const std::string& name) const noexcept;

    // Raw buffer containing armature node GPU data
    GLuint getNodesBuffer() const noexcept { return m_NodesBuffer; }

    // Number of nodes stored in the nodes buffer
    GLuint getNodesCount() const noexcept { return m_NodesCount; }

private:
    ArmatureNodeRefToIndexMap m_NodesRefToIndex;

    ArmatureNodeNameToIndexMap m_NodesNameToIndex;

    GLuint m_NodesBuffer;

    GLuint m_NodesCount;
};
