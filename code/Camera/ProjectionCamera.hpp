#pragma once

#include "Camera.hpp"

class ProjectionCamera :
    virtual public Camera {
public:
    constexpr static glm::float32 defaultFoVDegree = 60;

    explicit ProjectionCamera(
        const glm::float32& nearpln = Camera::defaultNearPlane,
        const glm::float32& farpln = Camera::defaultFarPlane,
        const glm::vec3& head = Camera::HeadUp,
        const glm::float32& fovDegree = defaultFoVDegree) noexcept;

    ProjectionCamera(const ProjectionCamera&) = delete;
    ProjectionCamera& operator=(const ProjectionCamera&) = delete;
    ~ProjectionCamera() override = default;

    void setFieldOfView(const glm::float32& fovDegree) const noexcept;

    glm::float32 getFieldOfView() const noexcept;

    glm::mat4 getProjectionMatrix(glm::uint32 width, glm::uint32 height) const noexcept final;

private:
    mutable glm::float32 fov;
};
