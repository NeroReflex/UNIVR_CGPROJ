#pragma once

#include "settings.hpp"
#include "Light/AmbientLight.hpp"
#include "Light/DirectionalLight.hpp"
#include "Light/ConeLight.hpp"
#include "Camera/Camera.hpp"
#include "Mesh.hpp"
#include "Animation.hpp"
#include "Armature.hpp"
#include "Pipeline.hpp"

#include "dds_loader/dds_header.hpp"

#include <filesystem>
#include <unordered_map>
#include <memory>
#include <vector>
#include <optional>
#include <array>
#include <functional>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>

class SceneElementAnimationStatus {

public:
    SceneElementAnimationStatus(
        const std::string& name
    ) noexcept;

    ~SceneElementAnimationStatus() = default;

    SceneElementAnimationStatus(const SceneElementAnimationStatus&) = delete;

    SceneElementAnimationStatus& operator=(const SceneElementAnimationStatus&) = delete;

    const std::string& getName(void) const noexcept;

    void advanceTime(float delta_time) noexcept;

    float getCurrentDeltaTime() const noexcept;

private:
    std::string m_name;

    float m_current_delta_time;
};

class SceneElement {
public:
    SceneElement(
        std::vector<std::shared_ptr<Mesh>>&& meshes,
        std::shared_ptr<SkeletonTree>&& skeleton,
        std::unordered_map<std::string, std::shared_ptr<Animation>>&& animations
    ) noexcept :
        m_meshes(std::move(meshes)),
        m_skeleton(skeleton),
        m_animations(animations)
    {}

    ~SceneElement() = default;

    SceneElement(const SceneElement&) = delete;

    SceneElement& operator=(const SceneElement&) = delete;

    void foreachMesh(std::function<void(const Mesh&)> fn) const noexcept;

    std::shared_ptr<Animation> getCurrentAnimation(void) const noexcept;

    std::optional<std::string> getCurrentAnimationName(void) const noexcept;

    inline std::shared_ptr<Armature> getArmature(void) const noexcept { return m_skeleton->getArmature(); }

    inline std::shared_ptr<SkeletonTree> getSkeleton(void) const noexcept { return m_skeleton; }

    bool hasAnimations(void) const noexcept;

    const std::unordered_map<std::string, std::shared_ptr<Animation>>& getAnimations() const noexcept;

    void advanceTime(float delta_time) noexcept;

    bool startAnimation(const std::string& name) noexcept;

    std::optional<double> getAnimationTime(void) const noexcept;

private:
    std::optional<SceneElementAnimationStatus> m_animation_status;

    std::vector<std::shared_ptr<Mesh>> m_meshes;

    std::unordered_map<std::string, std::shared_ptr<Animation>> m_animations;

    std::shared_ptr<SkeletonTree> m_skeleton;
};

typedef std::string SceneElementReference;

class Scene {

public:
    Scene(
        std::unique_ptr<Program>&& animation_compute_program,
        std::unique_ptr<Program>&& bind_pose_compute_program
    ) noexcept;

    ~Scene() = default;

    Scene(const Scene&) = delete;

    Scene& operator=(const Scene&) = delete;

    void update(double deltaTime) noexcept;

    void render(Pipeline *const pipeline) const noexcept;

    std::optional<SceneElementReference> load_asset(
        const std::string& name,
        const char *const asset_name,
        const glm::mat4& model = glm::mat4(1.0f)
    ) noexcept;

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

    bool startAnimation(
        const SceneElementReference& element_ref,
        const std::string& animation_name
    ) noexcept;

    std::optional<std::string> getRunningAnimationName(
        const SceneElementReference& element_ref
    ) const noexcept;
    std::optional<std::string> getAnimationName(
        const SceneElementReference& element_ref,
        size_t index
    ) const noexcept;

    static Scene* CreateScene() noexcept;

private:
    std::shared_ptr<Texture> assimp_load_texture(
        const std::filesystem::path& base_path,
        const aiMaterial *const assimp_material,
        aiTextureType assimp_type
    ) noexcept;

    static std::shared_ptr<Texture> load_texture(const std::filesystem::path& texture_path) noexcept;

    std::unordered_map<std::string, std::shared_ptr<Texture>> m_texture_cache;

    std::unordered_map<SceneElementReference, std::unique_ptr<SceneElement>> m_elements;

    std::optional<AmbientLight> m_ambient_light;

    std::unordered_map<std::string, DirectionalLight> m_directional_lights;

    std::unordered_map<std::string, ConeLight> m_cone_lights;

    std::shared_ptr<Camera> m_camera;

    std::unique_ptr<Program> m_animation_compute_program;
    std::unique_ptr<Program> m_bind_pose_compute_program;
};
