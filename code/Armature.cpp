#include "Armature.hpp"
#include "SkeletonTree.hpp"

#include <iostream>

ArmatureNode::ArmatureNode(
    std::weak_ptr<ArmatureNode> parent,
    glm::mat4 transform,
    const std::string& name,
    ArmatureNodeRef node_ref
) noexcept :
    m_Name(name),
    m_NodeRef(node_ref),
    m_transform(transform),
    m_Parent(parent),
    m_Children()
{

}

void ArmatureNode::addChild(std::shared_ptr<ArmatureNode> child) noexcept {
    m_Children.push_back(child);
}

uint32_t ArmatureNode::countNodes(void) const noexcept {
    uint32_t count = 1; // Count this node
    for (const auto& child : m_Children) {
        count += child->countNodes();
    }

    return count;
}

/*
uint32_t ArmatureNode::countNodesWithValidBones(void) const noexcept {
    uint32_t count = m_BoneIndex.has_value() ? 1 : 0; // Count this node
    for (const auto& child : m_Children) {
        count += child->countNodesWithValidBones();
    }

    return count;
}
*/

ArmatureNode* ArmatureNode::findArmatureNodeByNameRecursive(
    const std::string& name
) {
    if (m_Name == name) {
        return this;
    }

    for (const auto& child : m_Children) {
        const auto result = child->findArmatureNodeByNameRecursive(name);
        if (result != nullptr) {
            return result;
        }
    }

    return nullptr;
}

Armature::Armature(
    ArmatureNodeRefToIndexMap&& nodes_ref_to_index,
    ArmatureNodeNameToIndexMap&& nodes_name_to_index,
    GLuint nodes_buffer,
    GLuint nodes_count
) noexcept :
    m_NodesRefToIndex(std::move(nodes_ref_to_index)),
    m_NodesNameToIndex(std::move(nodes_name_to_index)),
    m_NodesBuffer(nodes_buffer),
    m_NodesCount(nodes_count)
{

}

std::optional<uint32_t> Armature::findArmatureNodeByName(const std::string& name) const noexcept {
    if (m_NodesNameToIndex.contains(name)) {
        return m_NodesNameToIndex.at(name);
    }

    return std::nullopt;
}

std::optional<uint32_t> Armature::findArmatureNode(const ArmatureNodeRef& node_ref) const noexcept {
    if (m_NodesRefToIndex.contains(node_ref)) {
        return m_NodesRefToIndex.at(node_ref);
    }

    return std::nullopt;
}

uint32_t ArmatureNode::write_armature_data(
    ArmatureNodeRefToIndexMap& armature_node_to_index_map,
    ArmatureNodeNameToIndexMap& armature_node_name_to_index_map,
    std::unordered_map<uintptr_t, uint32_t>& armature_node_to_index,
    GLuint buffer,
    uint32_t parent_index,
    uint32_t& nodes_count
) {
    const uintptr_t this_node_ptr = reinterpret_cast<uintptr_t>(this);
    
    assert(!armature_node_to_index.contains(this_node_ptr) && "Armature node already processed");
    // Avoid processing the same node multiple times
    //if (armature_node_to_index.contains(this_node_ptr)) {
    //    return armature_node_to_index[this_node_ptr];
    //}

    // in teoria dovrebbe funzionare, ma se ho così tanti elementi nella scena forse è meglio pensarci un attimo
    assert (nodes_count < 4096u && "Exceeded maximum number of armature nodes");

    const auto current_node_index = nodes_count;
    ++nodes_count;
    armature_node_to_index_map[m_NodeRef] = current_node_index;
    armature_node_to_index[this_node_ptr] = current_node_index;
    armature_node_name_to_index_map[m_Name] = armature_node_to_index[this_node_ptr];

    ArmatureGPUElement gpu_element = {
        .transform = m_transform,
        .parent_index = parent_index,
    };

    const auto gpu_index = armature_node_to_index[this_node_ptr];
    for (uint32_t i = 0; i < m_Children.size(); ++i) {
        /*gpu_element.childs[i] =*/ m_Children[i]->write_armature_data(
            armature_node_to_index_map,
            armature_node_name_to_index_map,
            armature_node_to_index,
            buffer,
            gpu_index,
            nodes_count
        );
    }

    // TODO: update the GPU buffer with gpu_element data at the correct offset
    CHECK_GL_ERROR(glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer));
    CHECK_GL_ERROR(glBufferSubData(
        GL_SHADER_STORAGE_BUFFER,
        static_cast<GLsizeiptr>(gpu_index * sizeof(ArmatureGPUElement)),
        sizeof(ArmatureGPUElement),
        &gpu_element
    ));
    CHECK_GL_ERROR(glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0));

    return armature_node_to_index[this_node_ptr];
}

Armature* Armature::CreateArmature(
    std::shared_ptr<ArmatureNode> root_node
) noexcept {
    GLuint buffer = 0;
    CHECK_GL_ERROR(glGenBuffers(1, &buffer));
    // Create a shader storage buffer (SSBO) to hold the skeleton data.
    CHECK_GL_ERROR(glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer));
    CHECK_GL_ERROR(glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        static_cast<GLsizeiptr>(root_node->countNodes() * sizeof(ArmatureGPUElement)),
        nullptr,
        GL_STATIC_DRAW
    ));
    CHECK_GL_ERROR(glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0));

    uint32_t nodes_count = 0;
    std::unordered_map<uintptr_t, uint32_t> armature_node_to_index;
    ArmatureNodeRefToIndexMap armature_node_to_index_map;
    ArmatureNodeNameToIndexMap armature_node_name_to_index_map;

    const uint32_t HARDCODED_ROOT_INDEX = 0u;
    uint32_t root_index = root_node->write_armature_data(
        armature_node_to_index_map,
        armature_node_name_to_index_map,
        armature_node_to_index,
        buffer,
        HARDCODED_ROOT_INDEX,
        nodes_count
    );
    assert(root_index == HARDCODED_ROOT_INDEX && "Root node must have index 0");

    const auto indexed_nodes_count = armature_node_to_index_map.size();

    std::cout << "Armature created with " << nodes_count << " nodes (" << indexed_nodes_count << " indexed)" << std::endl;

    assert(indexed_nodes_count == nodes_count && "All armature nodes must be indexed (by ref)");
    assert(armature_node_name_to_index_map.size() == nodes_count && "All armature nodes must be indexed (by string)");

    return new Armature(
        std::move(armature_node_to_index_map),
        std::move(armature_node_name_to_index_map),
        buffer,
        nodes_count
    );
}
