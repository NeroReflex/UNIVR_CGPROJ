#pragma once

#include "Camera.hpp"

class OrthoCamera :
    virtual public Camera {
public:
    constexpr static glm::float32 defaultOrthoHeight = 1.0f;

    explicit OrthoCamera(
        const glm::float32& nearpln = Camera::defaultNearPlane,
        const glm::float32& farpln = Camera::defaultFarPlane,
        const glm::vec3& head = Camera::HeadUp,
        const glm::float32& orthoHeight = defaultOrthoHeight) noexcept;

    OrthoCamera(const OrthoCamera&) = delete;
    OrthoCamera& operator=(const OrthoCamera&) = delete;
    ~OrthoCamera() override = default;

    void setOrthoHeight(const glm::float32& orthoHeight) const noexcept;

    glm::float32 getOrthoHeight() const noexcept;

    glm::mat4 getProjectionMatrix(glm::uint32 width, glm::uint32 height) const noexcept final;

private:
    mutable glm::float32 height;
};
