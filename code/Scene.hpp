#pragma once

#include "settings.hpp"
#include "Light/AmbientLight.hpp"
#include "Light/DirectionalLight.hpp"
#include "Light/ConeLight.hpp"
#include "Camera/Camera.hpp"
#include "Mesh.hpp"

#include "dds_loader/dds_header.hpp"

#include <filesystem>
#include <unordered_map>
#include <memory>
#include <vector>
#include <optional>
#include <array>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/material.h>
#include <assimp/postprocess.h>

class Scene {

public:
    Scene() noexcept;

    ~Scene();

    void render() const noexcept;

    void load_asset(const char *const asset_name) noexcept;

    void setCamera(std::shared_ptr<Camera> camera) noexcept;

    std::shared_ptr<Camera> getCamera() const noexcept;

    void foreachMesh(std::function<void(const Mesh&)> fn) const noexcept;

    void setAmbientLight(const AmbientLight& ambient_light) noexcept;

    const AmbientLight* getAmbientLight() const noexcept;

    void removeAmbientLight(void) noexcept;

    void setDirectionalLight(const std::string& name, const DirectionalLight& light) noexcept;

    const DirectionalLight* getDirectionalLight(const std::string& name) noexcept;

    void removeDirectionalLight(const std::string& name) noexcept;

    void foreachDirectionalLight(
        const std::function<void(const DirectionalLight&)>& fn
    ) const noexcept;

    void setConeLight(const std::string& name, const ConeLight& light) noexcept;

    const ConeLight* getConeLight(const std::string& name) noexcept;

    void removeConeLight(const std::string& name) noexcept;

    void foreachConeLight(
        const std::function<void(const ConeLight&)>& fn
    ) const noexcept;

private:
    std::shared_ptr<Texture> assimp_load_texture(
        const std::filesystem::path& base_path,
        const aiMaterial *const assimp_material,
        aiTextureType assimp_type
    ) noexcept;

    static std::shared_ptr<Texture> load_texture(const std::filesystem::path& texture_path) noexcept;

    std::unordered_map<std::string, std::shared_ptr<Texture>> m_texture_cache;

    std::vector<std::unique_ptr<Mesh>> m_meshes;

    std::optional<AmbientLight> m_ambient_light;

    std::unordered_map<std::string, DirectionalLight> m_directional_lights;

    std::unordered_map<std::string, ConeLight> m_cone_lights;

    std::shared_ptr<Camera> m_camera;
};
