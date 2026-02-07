#include "Scene.hpp"

#include <assert.h>
#include <iostream>
#include <filesystem>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <optional>

#include "Scene.hpp"

#include <assert.h>
#include <iostream>
#include <filesystem>
#include <vector>
#include <cstdint>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "dds_loader/dds_loader.hpp"

#include "embedded_shaders_symbols.h"

SceneElementAnimationStatus::SceneElementAnimationStatus(
    const std::string& name
) noexcept :
    m_name(name),
    m_current_delta_time(0.0)
{

}

void SceneElementAnimationStatus::advanceTime(float delta_time) noexcept {
    m_current_delta_time += delta_time;
}

const std::string& SceneElementAnimationStatus::getName(void) const noexcept {
    return m_name;
}

float SceneElementAnimationStatus::getCurrentDeltaTime() const noexcept {
    return m_current_delta_time;
}

void SceneElement::foreachMesh(std::function<void(const Mesh&)> fn) const noexcept {
    for (const auto& mesh : m_meshes) {
        fn(*mesh);
    }
}

bool SceneElement::hasAnimations(void) const noexcept {
    return !m_animations.empty();
}

const std::unordered_map<std::string, std::shared_ptr<Animation>>& SceneElement::getAnimations() const noexcept {
    return m_animations;
}

std::shared_ptr<Animation> SceneElement::getCurrentAnimation() const noexcept {
    if (!m_animation_status.has_value()) return nullptr;
    const auto& name = m_animation_status->getName();
    const auto it = m_animations.find(name);
    if (it == m_animations.end()) return nullptr;
    return it->second;
}

std::optional<std::string> SceneElement::getCurrentAnimationName(void) const noexcept {
    if (!m_animation_status.has_value()) {
        return std::nullopt;
    }

    return m_animation_status->getName();
}
bool SceneElement::startAnimation(const std::string& name) noexcept {
    if (m_animation_status.has_value()) {
        std::cout << "Animation " << m_animation_status->getName() << " is already playing, cannot start another animation (" << name << ")" << std::endl;
        return false;
    }

    if (!m_animations.contains(name)) {
        return false;
    }

    m_animation_status.emplace(name);

    return true;
}

std::optional<double> SceneElement::getAnimationTime(void) const noexcept {
    if (m_animation_status.has_value()) {
        return m_animation_status->getCurrentDeltaTime();
    }

    return std::nullopt;
}

void SceneElement::advanceTime(float delta_time) noexcept {
    if (m_animation_status.has_value()) {
        m_animation_status->advanceTime(delta_time);

        const auto& name = m_animation_status->getName();
        const auto it = m_animations.find(name);
        if (it != m_animations.end() && it->second) {
            const auto anim = it->second;
            const double duration_ticks = anim->getDuration();
            const double ticks_per_second = anim->getTicksPerSecond();
            double duration_seconds = duration_ticks;
            if (ticks_per_second > 0.0) duration_seconds = duration_ticks / ticks_per_second;

            if (m_animation_status->getCurrentDeltaTime() >= static_cast<float>(duration_seconds)) {
                std::cout << "Animation " << name << " completed after " << m_animation_status->getCurrentDeltaTime() << "s." << std::endl;
                m_animation_status.reset();
            }
        }
    }
}

Scene::Scene(
    std::unique_ptr<Program>&& animation_compute_program,
    std::unique_ptr<Program>&& bind_pose_compute_program
) noexcept :
    m_elements(),
    m_ambient_light(),
    m_camera(nullptr),
    m_animation_compute_program(std::move(animation_compute_program)),
    m_bind_pose_compute_program(std::move(bind_pose_compute_program))
{

}

void Scene::render(Pipeline *const pipeline) const noexcept {
    // TODO: update animations

    pipeline->render(this);
}

// indice=verticeID, elemento=lista di (boneIndex, weight)
typedef std::unordered_map<uint32_t, std::vector<std::tuple<uint32_t, float>>> VertexBoneDataMap;

/**
 * 
 * @param skeleton_metadata indice=bone name, element=(bone parent name, bone transform)
 */
VertexBoneDataMap load_bones_for_mesh(
    const aiScene *const scene,
    const aiMesh *pMesh,
    std::shared_ptr<SkeletonTree>& skeleton_metadata
) {
    const auto mesh_name = std::string(pMesh->mName.C_Str());

    // Loop through all bones in the Assimp mesh and ensure they exist in skeleton_metadata.
    VertexBoneDataMap vertex_to_bone;
    for (unsigned int i = 0; i < pMesh->mNumBones; i++) {
        const auto bone = pMesh->mBones[i];
        const auto bone_name = std::string(bone->mName.data);

        const ArmatureNodeRef armature_node_ref = reinterpret_cast<ArmatureNodeRef>(bone->mNode);
        assert(armature_node_ref != 0 && "Bone has no corresponding node in the Assimp scene hierarchy");

        // add bone to skeleton metadata if not already present
        const auto result = skeleton_metadata->addBone(
            armature_node_ref,
            glm::transpose(glm::make_mat4(&bone->mOffsetMatrix.a1))
        );

        assert(result && "Failed to add bone to skeleton metadata");

        // Cerca l'osso nell'elenco delle ossa già caricate
        const auto bone_search_result = skeleton_metadata->getBoneIndex(armature_node_ref);
        assert(bone_search_result.has_value() && "Bone not found in skeleton metadata after addition");

        const auto bone_index = bone_search_result.value();

        std::cout << "Bone " << bone_name << " in mesh " << mesh_name << std::endl;

        for (unsigned int j = 0; j < bone->mNumWeights; j++) {
            uint32_t vertexID = bone->mWeights[j].mVertexId;
            float weight = bone->mWeights[j].mWeight;

            const auto data = std::make_tuple(bone_index, weight);
            if (vertex_to_bone.contains(vertexID)) {
                vertex_to_bone[vertexID].push_back(data);
            } else {
                vertex_to_bone[vertexID] = { data };
            }
        }
    }

    return vertex_to_bone;
}

static std::shared_ptr<Animation> process_animation(
    const aiScene *const scene,
    const aiAnimation *const assimp_animation,
    const std::shared_ptr<Armature>& meshes_armature
) {
    const auto animation_shared_ptr = std::shared_ptr<Animation>(
        Animation::CreateAnimation(
            assimp_animation->mDuration,
            assimp_animation->mTicksPerSecond,
            meshes_armature
        )
    );

    glm::uint32 max_key_per_channel = 0;
    for (unsigned int c = 0; c < assimp_animation->mNumChannels; ++c) {
        const auto animated_bone = assimp_animation->mChannels[c];
        const auto animated_bone_name = std::string(animated_bone->mNodeName.C_Str());

        const auto armature_element_index_opt = meshes_armature->findArmatureNodeByName(animated_bone_name);
        assert(armature_element_index_opt.has_value() && "Animation channel node not found in armature");

        const auto debug = glm::max(animated_bone->mNumPositionKeys, glm::max(animated_bone->mNumRotationKeys, animated_bone->mNumScalingKeys));
        max_key_per_channel = glm::max(max_key_per_channel, debug);

        animation_shared_ptr->addChannel(
            animated_bone_name,
            // position keys
            [&animated_bone]() {
                std::vector<std::tuple<double, glm::vec3>> keys;
                for (uint32_t pk = 0; pk < animated_bone->mNumPositionKeys; ++pk) {
                    const auto& position_key = animated_bone->mPositionKeys[pk];
                    keys.emplace_back(
                        position_key.mTime,
                        glm::vec3(position_key.mValue.x, position_key.mValue.y, position_key.mValue.z)
                    );
                }
                return keys;
            }(),
            // rotation keys
            [&animated_bone]() {
                std::vector<std::tuple<double, glm::quat>> keys;
                for (uint32_t rk = 0; rk < animated_bone->mNumRotationKeys; ++rk) {
                    const auto& rotation_key = animated_bone->mRotationKeys[rk];
                    keys.emplace_back(
                        rotation_key.mTime,
                        glm::quat(rotation_key.mValue.w, rotation_key.mValue.x, rotation_key.mValue.y, rotation_key.mValue.z)
                    );
                }
                return keys;
            }(),
            // scaling keys
            [&animated_bone]() {
                std::vector<std::tuple<double, glm::vec3>> keys;
                for (uint32_t sk = 0; sk < animated_bone->mNumScalingKeys; ++sk) {
                    const auto& scaling_key = animated_bone->mScalingKeys[sk];
                    keys.emplace_back(
                        scaling_key.mTime,
                        glm::vec3(scaling_key.mValue.x, scaling_key.mValue.y, scaling_key.mValue.z)
                    );
                }
                return keys;
            }()
        );
    }

    return animation_shared_ptr;
}

static std::shared_ptr<ArmatureNode> process_armature(
    const aiNode *const assimp_node,
    std::weak_ptr<ArmatureNode> parent
) {
    const auto armature_node_ref = reinterpret_cast<ArmatureNodeRef>(assimp_node);
    const auto node_name = std::string(assimp_node->mName.C_Str());

    auto armature_node = std::make_shared<ArmatureNode>(
        parent,
        glm::transpose(glm::make_mat4(&assimp_node->mTransformation.a1)),
        node_name,
        armature_node_ref
    );
    for (unsigned int i = 0; i < assimp_node->mNumChildren; ++i) {
        const auto child_node = process_armature(
            assimp_node->mChildren[i],
            armature_node
        );

        assert(child_node != nullptr && "Child armature node is null");
        armature_node->addChild(child_node);
    }

    return armature_node;
}

std::optional<SceneElementReference> Scene::load_asset(
    const std::string& name,
    const char *const asset_name,
    const glm::mat4& model
) noexcept {
    std::filesystem::path asset_path(asset_name);
    if (!std::filesystem::exists(asset_path)) {
        std::cerr << "Asset file does not exist: " << asset_name << std::endl;
        return std::nullopt;
    }

    // aiProcess_CalcTangentSpace can be added if tangents are needed in the vertex shader
    Assimp::Importer importer;
    const aiScene *const scene = importer.ReadFile(
        asset_name,
        (aiProcessPreset_TargetRealtime_Quality & ~aiProcess_SplitLargeMeshes) |
        aiProcess_PopulateArmatureData |
        aiProcess_LimitBoneWeights |
        aiProcess_SortByPType |
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenSmoothNormals
    );
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        // Handle error
        return std::nullopt;
    }

    std::cout << "Successfully loaded " << scene->mRootNode->mNumChildren << " child nodes from asset: " << asset_name << std::endl;

    const auto armature_tree = process_armature(
        scene->mRootNode,
        std::shared_ptr<ArmatureNode>(nullptr)
    );
    assert(armature_tree != nullptr && "Failed to process armature tree");

    // process the armature data: create the root node of the armature hierarchy
    auto meshes_armature = std::shared_ptr<Armature>(
        Armature::CreateArmature(
            armature_tree
        )
    );

    // usato per tenere traccia delle ossa visitate
    auto skeleton_tree = std::shared_ptr<SkeletonTree>(
        SkeletonTree::CreateSkeletonTree(meshes_armature)
    );

    assert(skeleton_tree != nullptr && "Failed to create SkeletonTree");

    std::vector<std::shared_ptr<Mesh>> meshes;

    // Process the scene's root node recursively
    for (unsigned int j = 0; j < scene->mNumMeshes; j++) {
        const auto *const mesh = scene->mMeshes[j];

        // Create and fill VBO using buffer mapping (position.xyz [+ normal.xyz] + texcoord.uv interleaved)
        const GLuint vertex_count = mesh->mNumVertices;
        const GLsizeiptr vertex_buffer_size = static_cast<GLsizeiptr>(vertex_count * sizeof(VertexData));

        VertexBoneDataMap vertex_bone_data;
        if (mesh->HasBones()) {
            vertex_bone_data = load_bones_for_mesh(scene, mesh, skeleton_tree);
        }

        const GLuint vbo = Mesh::CreateVertexBuffer(nullptr, vertex_buffer_size);

        // Determine whether the mesh provides normals; if not we'll compute per-vertex normals
        const bool hasNormals = mesh->HasNormals() != 0;
        std::vector<glm::vec3> computed_normals;
        if (!hasNormals) {
            computed_normals.assign(vertex_count, glm::vec3(0.0f));
            for (unsigned int fi = 0; fi < mesh->mNumFaces; ++fi) {
                const aiFace &face = mesh->mFaces[fi];
                if (face.mNumIndices < 3) continue;
                const unsigned int ia = face.mIndices[0];
                const unsigned int ib = face.mIndices[1];
                const unsigned int ic = face.mIndices[2];
                const aiVector3D &pa = mesh->mVertices[ia];
                const aiVector3D &pb = mesh->mVertices[ib];
                const aiVector3D &pc = mesh->mVertices[ic];
                const glm::vec3 a(pa.x, pa.y, pa.z);
                const glm::vec3 b(pb.x, pb.y, pb.z);
                const glm::vec3 c(pc.x, pc.y, pc.z);
                const glm::vec3 face_normal = glm::normalize(glm::cross(b - a, c - a));
                computed_normals[ia] += face_normal;
                computed_normals[ib] += face_normal;
                computed_normals[ic] += face_normal;
            }
        }

        // map and write vertex data directly into GPU memory
        {
            VertexData *const vptr = (VertexData*)glMapBufferRange(
                GL_ARRAY_BUFFER,
                0,
                vertex_buffer_size,
                GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT
            );
            assert(vptr != nullptr && "Failed to map vertex buffer");

            for (unsigned int vi = 0; vi < mesh->mNumVertices; ++vi) {
                VertexData *dest = &vptr[vi];

                const aiVector3D &pos = mesh->mVertices[vi];
                dest->position_x = pos.x;
                dest->position_y = pos.y;
                dest->position_z = pos.z;

                // normal
                if (hasNormals) {
                    const aiVector3D &n = mesh->mNormals[vi];
                    dest->normal_x = n.x;
                    dest->normal_y = n.y;
                    dest->normal_z = n.z;
                } else {
                    const glm::vec3 &n = computed_normals[vi];
                    dest->normal_x = n.x;
                    dest->normal_y = n.y;
                    dest->normal_y = n.z;
                }

                // texcoord
                if (mesh->mTextureCoords[0]) {
                    const aiVector3D &uv = mesh->mTextureCoords[0][vi];
                    dest->texcoord_u = uv.x;
                    dest->texcoord_v = uv.y;
                } else {
                    dest->texcoord_u = 0.0f;
                    dest->texcoord_v = 0.0f;
                }

                // bone data
                if (vertex_bone_data.contains(vi)) {
                    const auto& bone_data = vertex_bone_data[vi];
                    
                    assert(bone_data.size() <= 4 && "A vertex cannot be influenced by more than 4 bones");

                    for (size_t bi = 0; bi < bone_data.size(); ++bi) {
                        const auto [bone_index, weight] = bone_data[bi];
                        switch (bi) {
                            case 0:
                                dest->bone_index_0 = bone_index;
                                dest->bone_weight_0 = weight;
                                break;
                            case 1:
                                dest->bone_index_1 = bone_index;
                                dest->bone_weight_1 = weight;
                                break;
                            case 2:
                                dest->bone_index_2 = bone_index;
                                dest->bone_weight_2 = weight;
                                break;
                            case 3:
                                dest->bone_index_3 = bone_index;
                                dest->bone_weight_3 = weight;
                                break;
                            default:
                                break;
                        }
                    }
                } else {
                    dest->bone_index_0 = BONE_IS_ROOT;
                    dest->bone_weight_0 = 0.0f;

                    dest->bone_index_1 = BONE_IS_ROOT;
                    dest->bone_weight_1 = 0.0f;

                    dest->bone_index_2 = BONE_IS_ROOT;
                    dest->bone_weight_2 = 0.0f;

                    dest->bone_index_3 = BONE_IS_ROOT;
                    dest->bone_weight_3 = 0.0f;
                }
            }

            glUnmapBuffer(GL_ARRAY_BUFFER);
        }

        // Build and fill index buffer using buffer mapping (assume triangulated faces)
        GLuint total_indices = 0;
        for (unsigned int fi = 0; fi < mesh->mNumFaces; ++fi) total_indices += mesh->mFaces[fi].mNumIndices;

        // Use 32-bit indices: large models (like many glTF exports) can exceed 65535 vertices.
        const GLsizeiptr index_buffer_size = static_cast<GLsizeiptr>(total_indices * sizeof(uint32_t));
        const GLuint ibo = Mesh::CreateElementBuffer(nullptr, index_buffer_size);

        {
            void *iptr = glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, index_buffer_size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
            assert(iptr != nullptr && "Failed to map index buffer");
            uint32_t *idst = static_cast<uint32_t*>(iptr);
            for (GLuint fi = 0; fi < mesh->mNumFaces; ++fi) {
                const aiFace &face = mesh->mFaces[fi];
                for (GLuint k = 0; k < face.mNumIndices; ++k) {
                    idst[0] = static_cast<uint32_t>(face.mIndices[k]);
                    ++idst;
                }
            }
            glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
        }

        float shininess = 0.0f;
        glm::vec3 diffuse_color(0.0f);
        glm::vec3 specular_color(0.0f);
        const aiMaterial *const assimp_mat = scene->mMaterials[mesh->mMaterialIndex];
        if (assimp_mat) {
            ai_real s = 0.0;
            if (assimp_mat->Get(AI_MATKEY_SHININESS, s) == AI_SUCCESS) {
                shininess = static_cast<float>(s);
            }

            aiColor3D dcol(0.0f, 0.0f, 0.0f);
            if (assimp_mat->Get(AI_MATKEY_COLOR_DIFFUSE, dcol) == AI_SUCCESS) {
                diffuse_color = glm::vec3(static_cast<float>(dcol.r), static_cast<float>(dcol.g), static_cast<float>(dcol.b));
            }

            aiColor3D scol(0.0f, 0.0f, 0.0f);
            if (assimp_mat->Get(AI_MATKEY_COLOR_SPECULAR, scol) == AI_SUCCESS) {
                specular_color = glm::vec3(static_cast<float>(scol.r), static_cast<float>(scol.g), static_cast<float>(scol.b));
            }
        }

        auto material = std::make_shared<Material>(diffuse_color, specular_color, shininess);
        material->setDiffuseTexture(
            assimp_load_texture(
                asset_path.parent_path(),
                scene->mMaterials[mesh->mMaterialIndex],
                aiTextureType_DIFFUSE
            )
        );

        material->setSpecularTexture(
            assimp_load_texture(
                asset_path.parent_path(),
                scene->mMaterials[mesh->mMaterialIndex],
                aiTextureType_SPECULAR
            )
        );

        material->setDisplacementTexture(
            assimp_load_texture(
                asset_path.parent_path(),
                scene->mMaterials[mesh->mMaterialIndex],
                aiTextureType_DISPLACEMENT
            )
        );

        // Store mesh with VBO and IBO (ibo_count = number of indices). Normals are always present (either loaded or generated).
        meshes.emplace_back(
            std::make_unique<Mesh>(
                vbo,
                ibo,
                static_cast<GLuint>(total_indices),
                material,
                skeleton_tree,
                model
            )
        );
    }

    std::unordered_map<std::string, std::shared_ptr<Animation>> animations_map;

    for (unsigned int a = 0; a < scene->mNumAnimations; ++a) {
        const auto animation = scene->mAnimations[a];

        const auto animation_name = (animation->mName.C_Str());

        std::cout << "Animation " << animation_name << " has duration " << animation->mDuration << " ticks at " << animation->mTicksPerSecond << " ticks/second." << std::endl;
    
        const auto animation_shared_ptr = std::shared_ptr<Animation>(
            process_animation(scene, animation, meshes_armature)
        );

        // store the animation
        animations_map[animation_name] = animation_shared_ptr;
    }

    m_elements[name] = std::move(
        std::make_unique<SceneElement>(
            std::move(meshes),
            std::move(skeleton_tree),
            std::move(animations_map)
        )
    );

    return name;
}

std::shared_ptr<Texture> Scene::assimp_load_texture(
    const std::filesystem::path& base_path,
    const aiMaterial *const assimp_material,
    aiTextureType assimp_type
) noexcept {
    // Process material properties and textures here as needed
    const auto diffuse_texture_count = assimp_material->GetTextureCount(assimp_type);
    std::shared_ptr<Texture> diffuse_texture(nullptr);
    //std::cout << "Material " <<  material->GetName().C_Str() << " has " << texture_count << " diffuse textures." << std::endl;
    for (unsigned int ti = 0; ti < diffuse_texture_count; ++ti) {
        aiString str;
        if (assimp_material->GetTexture(assimp_type, ti, &str) == AI_SUCCESS) {
            const auto current_texture_key = std::string(str.C_Str());
            const auto it = m_texture_cache.find(current_texture_key);
            diffuse_texture = (it == m_texture_cache.end()) ?
                load_texture(base_path / std::filesystem::path(str.C_Str())) :
                it->second;

            // Do not fail but print an error message
            //assert(diffuse_texture != nullptr && "Failed to load texture");
            if (!diffuse_texture) {
                std::cerr << "Failed to load texture: " << (base_path / std::filesystem::path(str.C_Str())) << std::endl;
                continue;
            }
            m_texture_cache.insert({current_texture_key, diffuse_texture});
        }
    }

    return diffuse_texture;
}

std::shared_ptr<Texture> Scene::load_texture(const std::filesystem::path& texture_path) noexcept {
    if (!std::filesystem::exists(texture_path)) {
        std::cerr << "Texture file does not exist: " << texture_path << std::endl;
        return nullptr;
    }

    // See https://github.com/KhronosGroup/3D-Formats-Guidelines/blob/main/KTXDeveloperGuide.md for a nice table of runtime GL formats

    const auto ext = texture_path.extension().string();
    std::string ext_lower(ext.begin(), ext.end());
    std::transform(ext_lower.begin(), ext_lower.end(), ext_lower.begin(), ::tolower);

    std::shared_ptr<Texture> loaded_texture(nullptr);
    if (ext_lower == ".dds") {
        DDSLoadResult load_result;
        const auto dds_resource = load_dds(texture_path, load_result);
        if (!dds_resource) {
            std::cerr << "Texture couldn't be loaded: " << (int)load_result << std::endl;
            return nullptr;
        }

        loaded_texture = std::shared_ptr<Texture>(
            Texture::CreateBC7Texture2D(
                GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_EXT,
                dds_resource->get_width(),
                dds_resource->get_height(),
                dds_resource->get_mipmap_count(),
                dds_resource->get_data()
            )
        );

        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            // Fallback: create a simple 1x1 white RGBA texture so the mesh is visible
            const uint8_t whitePixel[4] = { 255, 255, 255, 255 };
            CHECK_GL_ERROR({
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
                // Use simple filtering (no mipmaps)
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            });
        }

        // Unbind texture from the current unit to avoid accidental use
        glBindTexture(GL_TEXTURE_2D, 0);
    } else if (ext_lower == ".png" || ext_lower == ".jpg" || ext_lower == ".jpeg") {
        loaded_texture = std::shared_ptr<Texture>(
            Texture::Create2DTextureFromFile(texture_path.string(), TextureWrapMode::TEXTURE_WRAP_MODE_REPEAT, TextureWrapMode::TEXTURE_WRAP_MODE_REPEAT, TextureFilterMode::TEXTURE_FILTER_MODE_LINEAR, TextureFilterMode::TEXTURE_FILTER_MODE_LINEAR)
        );

        if (!loaded_texture) {
            std::cerr << "Failed to load image texture: " << texture_path << std::endl;
        }
    } else {
        std::cerr << "Texture couldn't be loaded (unsupported or invalid): " << texture_path << std::endl;
    }

    return loaded_texture;
}

void Scene::setCamera(std::shared_ptr<Camera> camera) noexcept {
    m_camera = camera;
}

std::shared_ptr<Camera> Scene::getCamera() const noexcept {
    return m_camera;
}

void Scene::foreachMesh(std::function<void(const Mesh&)> fn) const noexcept {
    for (const auto& element : m_elements) {
        element.second->foreachMesh(fn);
    }
}

std::optional<std::string> Scene::getRunningAnimationName(const SceneElementReference& element_ref) const noexcept {
    const auto it = m_elements.find(element_ref);
    if (it == m_elements.end()) return std::nullopt;
    return it->second->getCurrentAnimationName();
}

void Scene::setAmbientLight(const AmbientLight& ambient_light) noexcept {
    m_ambient_light = ambient_light;
}

const AmbientLight* Scene::getAmbientLight() const noexcept {
    if (m_ambient_light.has_value()) {
        return const_cast<AmbientLight*>(&m_ambient_light.value());
    }

    return nullptr;
}

void Scene::removeAmbientLight(void) noexcept {
    m_ambient_light.reset();
}

void Scene::setDirectionalLight(const std::string& name, const DirectionalLight& light) noexcept {
    m_directional_lights.insert_or_assign(name, light);
}

const DirectionalLight* Scene::getDirectionalLight(const std::string& name) noexcept {
    const auto it = m_directional_lights.find(name);
    if (it == m_directional_lights.end()) return nullptr;
    return &it->second;
}

void Scene::removeDirectionalLight(const std::string& name) noexcept {
    m_directional_lights.erase(name);
}

void Scene::foreachDirectionalLight(
    const std::function<void(const DirectionalLight&)>& fn
) const noexcept {
    for (const auto& light : m_directional_lights) {
        fn(light.second);
    }
}

void Scene::setConeLight(const std::string& name, const ConeLight& light) noexcept {
    m_cone_lights.insert_or_assign(name, light);
}

const ConeLight* Scene::getConeLight(const std::string& name) noexcept {
    const auto it = m_cone_lights.find(name);
    if (it == m_cone_lights.end()) return nullptr;
    return &it->second;
}

void Scene::removeConeLight(const std::string& name) noexcept {
    m_cone_lights.erase(name);
}

void Scene::foreachConeLight(
    const std::function<void(const ConeLight&)>& fn
) const noexcept {
    for (const auto& light : m_cone_lights) {
        fn(light.second);
    }
}

void Scene::update(double deltaTime) noexcept {
    for (auto& element : m_elements) {
        // Update animations
        element.second->advanceTime(static_cast<float>(deltaTime));

        // Update per-frame bone data here
        const auto anim_time = element.second->getAnimationTime();
        const auto skeleton = element.second->getSkeleton();
        if (!skeleton) continue;

        const auto armature = skeleton->getArmature();
        if (!armature) continue;

        const GLuint bones = static_cast<GLuint>(skeleton->getBoneCount());
        const GLuint local_size_x = 32u; // must match compute shader local size
        const GLuint groups_x = (bones == 0u) ? 1u : ((bones + local_size_x - 1u) / local_size_x);

        if (anim_time.has_value()) {
            // Use the animation compute shader when an animation is active
            m_animation_compute_program->bind();

            // Provide time to the animation compute shader in the same units as the animation keys (ticks)
            const auto anim = element.second->getCurrentAnimation();
            assert(anim && "Current animation must be available when animation time is valid");
            const float ticks_per_second = static_cast<float>(anim->getTicksPerSecond() > 0.0 ? anim->getTicksPerSecond() : 1.0);
            const float time_in_ticks = static_cast<float>(anim_time.value()) * ticks_per_second;
            m_animation_compute_program->uniformFloat("u_DeltaTime", time_in_ticks);

            // Provide number of valid animation channels to the compute shader
            m_animation_compute_program->uniformUint("u_AnimationChannelCount", anim->getChannelsCount());

            // Bind SSBOs (animate.comp expects OriginalSkeletonBuffer, PerFrameSkeletonBuffer, ArmatureBuffer, AnimationBuffer)
            m_animation_compute_program->uniformStorageBufferBinding("OriginalSkeletonBuffer", skeleton->getOriginalBuffer());
            m_animation_compute_program->uniformStorageBufferBinding("PerFrameSkeletonBuffer", skeleton->getPerFrameBuffer());
            m_animation_compute_program->uniformStorageBufferBinding("ArmatureBuffer", armature->getNodesBuffer());
            m_animation_compute_program->uniformStorageBufferBinding("AnimationBuffer", anim->getChannelsBuffer());

            m_animation_compute_program->dispatchCompute(groups_x, 1, 1);
        } else {
            // No animation active -> compute bind-pose per-frame skeleton
            m_bind_pose_compute_program->bind();
            m_bind_pose_compute_program->uniformStorageBufferBinding("OriginalSkeletonBuffer", skeleton->getOriginalBuffer());
            m_bind_pose_compute_program->uniformStorageBufferBinding("PerFrameSkeletonBuffer", skeleton->getPerFrameBuffer());
            m_bind_pose_compute_program->uniformStorageBufferBinding("ArmatureBuffer", armature->getNodesBuffer());

            m_bind_pose_compute_program->dispatchCompute(groups_x, 1, 1);
        }
    }
}

bool Scene::startAnimation(
    const SceneElementReference& element_ref,
    const std::string& animation_name
) noexcept {
    auto it = m_elements.find(element_ref);
    if (it == m_elements.end()) {
        std::cerr << "Scene element " << element_ref << " not found." << std::endl;
        return false;
    }

    SceneElement *const element = it->second.get();
    if (!element->startAnimation(animation_name)) {
        std::cerr << "Failed to start animation " << animation_name << " on element " << element_ref << "." << std::endl;
        return false;
    }

    return true;
}

std::optional<std::string> Scene::getAnimationName(
    const SceneElementReference& element_ref,
    size_t index
) const noexcept {
    auto it = m_elements.find(element_ref);
    if (it == m_elements.end()) {
        std::cerr << "Scene element " << element_ref << " not found." << std::endl;
        return std::nullopt;
    }

    const SceneElement *const element = it->second.get();
    const auto& animations = element->getAnimations();

    if (index >= animations.size()) {
        std::cerr << "Animation index " << index << " out of range for element " << element_ref << "." << std::endl;
        return std::nullopt;
    }

    auto anim_it = animations.begin();
    std::advance(anim_it, index);
    return anim_it->first;
}

static std::string animation_shader_source_str(reinterpret_cast<const char*>(animate_comp_glsl), animate_comp_glsl_len);
static const GLchar *const animate_comp_shader_source = animation_shader_source_str.c_str();

static std::string bindpose_shader_source_str(reinterpret_cast<const char*>(animate_bind_pose_comp_glsl), animate_bind_pose_comp_glsl_len);
static const GLchar *const animate_bind_pose_comp_shader_source = bindpose_shader_source_str.c_str();

Scene* Scene::CreateScene() noexcept {
    std::unique_ptr<ComputeShader> animation_compute_shader(
        ComputeShader::CompileShader(
            reinterpret_cast<const char*>(animate_comp_shader_source)
        )
    );

    assert(animation_compute_shader != nullptr && "Failed to compile animation compute shader");

    std::unique_ptr<Program> animation_compute_program(
        Program::LinkProgram(
            animation_compute_shader.get()
        )
    );

    assert(animation_compute_program != nullptr && "Failed to link animation compute shader program");

    std::unique_ptr<ComputeShader> bindpose_compute_shader(
        ComputeShader::CompileShader(
            reinterpret_cast<const char*>(animate_bind_pose_comp_shader_source)
        )
    );
    assert(bindpose_compute_shader != nullptr && "Failed to compile bind-pose compute shader");

    std::unique_ptr<Program> bindpose_compute_program(
        Program::LinkProgram(
            bindpose_compute_shader.get()
        )
    );
    assert(bindpose_compute_program != nullptr && "Failed to link bind-pose compute shader program");

    return new Scene(std::move(animation_compute_program), std::move(bindpose_compute_program));
}
