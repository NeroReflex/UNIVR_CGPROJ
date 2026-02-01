#include "Scene.hpp"

#include <assert.h>
#include <iostream>
#include <filesystem>
#include <vector>
#include <cstdint>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "dds_loader/dds_loader.hpp"

Scene::Scene() noexcept :
    m_meshes(),
    m_ambient_light(),
    m_camera(nullptr)
{

}

Scene::~Scene() {
    // m_camera, m_meshes, and textures will be automatically released
}

void Scene::render() const noexcept {
    

}

void Scene::load_asset(const char *const asset_name, const glm::mat4& model) noexcept {
    std::filesystem::path asset_path(asset_name);
    if (!std::filesystem::exists(asset_path)) {
        std::cerr << "Asset file does not exist: " << asset_name << std::endl;
        return;
    }

    // aiProcess_CalcTangentSpace can be added if tangents are needed in the vertex shader
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(asset_name, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        // Handle error
        return;
    }

    std::cout << "Successfully loaded " << scene->mRootNode->mNumChildren << " child nodes from asset: " << asset_name << std::endl;

    // Process the scene's root node recursively
    for (unsigned int i = 0; i < scene->mRootNode->mNumChildren; i++) {
        const auto *const childNode = scene->mRootNode->mChildren[i];
        // Process each child node (e.g., load meshes, materials, etc.)

        for (unsigned int j = 0; j < childNode->mNumMeshes; j++) {
            unsigned int meshIndex = childNode->mMeshes[j];
            const auto *const mesh = scene->mMeshes[meshIndex];
            // Process the mesh (e.g., extract vertices, normals, texture coordinates, etc.)

            //std::cout << "Mesh " << meshIndex << " has " << mesh->mNumVertices << " vertices." << std::endl;

            // Determine whether the mesh provides normals; if not we'll compute per-vertex normals
            const bool hasNormals = mesh->HasNormals() != 0;

            // Create and fill VBO using buffer mapping (position.xyz [+ normal.xyz] + texcoord.uv interleaved)
            const GLuint vertex_count = mesh->mNumVertices;
            const GLsizeiptr vertex_buffer_size = static_cast<GLsizeiptr>(vertex_count * 8u * sizeof(float));

            const GLuint vbo = Mesh::CreateVertexBuffer(nullptr, vertex_buffer_size);

            // If normals are not provided by Assimp, compute them from faces
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
                void *vptr = glMapBufferRange(GL_ARRAY_BUFFER, 0, vertex_buffer_size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
                assert(vptr != nullptr && "Failed to map vertex buffer");
                float *dest = static_cast<float*>(vptr);
                for (unsigned int vi = 0; vi < mesh->mNumVertices; ++vi) {
                    const aiVector3D &pos = mesh->mVertices[vi];
                    *dest++ = pos.x;
                    *dest++ = pos.y;
                    *dest++ = pos.z;

                    // normal
                    if (hasNormals) {
                        const aiVector3D &n = mesh->mNormals[vi];
                        *dest++ = n.x;
                        *dest++ = n.y;
                        *dest++ = n.z;
                    } else {
                        const glm::vec3 &n = computed_normals[vi];
                        *dest++ = n.x;
                        *dest++ = n.y;
                        *dest++ = n.z;
                    }

                    // texcoord
                    if (mesh->mTextureCoords[0]) {
                        const aiVector3D &uv = mesh->mTextureCoords[0][vi];
                        *dest++ = uv.x;
                        *dest++ = uv.y;
                    } else {
                        *dest++ = 0.0f;
                        *dest++ = 0.0f;
                    }
                }
                glUnmapBuffer(GL_ARRAY_BUFFER);
            }

            // Build and fill index buffer using buffer mapping (assume triangulated faces)
            GLuint total_indices = 0;
            for (unsigned int fi = 0; fi < mesh->mNumFaces; ++fi) total_indices += mesh->mFaces[fi].mNumIndices;

            const GLsizeiptr index_buffer_size = static_cast<GLsizeiptr>(total_indices * sizeof(uint16_t));
            const GLuint ibo = Mesh::CreateElementBuffer(nullptr, index_buffer_size);

            {
                void *iptr = glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, index_buffer_size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
                assert(iptr != nullptr && "Failed to map index buffer");
                uint16_t *idst = static_cast<uint16_t*>(iptr);
                for (GLuint fi = 0; fi < mesh->mNumFaces; ++fi) {
                    const aiFace &face = mesh->mFaces[fi];
                    for (GLuint k = 0; k < face.mNumIndices; ++k) {
                        idst[0] = static_cast<uint16_t>(face.mIndices[k]);
                        ++idst;
                    }
                }
                glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
            }

            float shininess = 0.0f;
            const aiMaterial *const assimp_mat = scene->mMaterials[mesh->mMaterialIndex];
            if (assimp_mat) {
                ai_real s = 0.0;
                if (assimp_mat->Get(AI_MATKEY_SHININESS, s) == AI_SUCCESS) {
                    std::cout << "Mesh " << meshIndex << " has shininess: " << s << std::endl;
                    shininess = static_cast<float>(s);
                }
            }

            auto material = std::make_shared<Material>(glm::vec3(0.0), shininess);
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
            m_meshes.emplace_back(std::make_unique<Mesh>(vbo, ibo, static_cast<GLuint>(total_indices), material, model));
        }

    }
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

            assert(diffuse_texture != nullptr && "Failed to load texture");

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

    // load the texture and check for errors
    DDSLoadResult load_result;
    const auto dds_resource = load_dds(texture_path, load_result);
    if (!dds_resource) {
        std::cerr << "Texture couldn't be loaded: " << (int)load_result << std::endl;
        return nullptr;
    }

    // See https://github.com/KhronosGroup/3D-Formats-Guidelines/blob/main/KTXDeveloperGuide.md for a nice table of runtime GL formats
    const auto texture = std::shared_ptr<Texture>(
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

    return texture;
}

void Scene::setCamera(std::shared_ptr<Camera> camera) noexcept {
    m_camera = camera;
}

std::shared_ptr<Camera> Scene::getCamera() const noexcept {
    return m_camera;
}

void Scene::foreachMesh(std::function<void(const Mesh&)> fn) const noexcept {
    for (const auto& mesh : m_meshes) {
        fn(*mesh);
    }
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
