#pragma once

#include "Camera.hpp"

class RotativeCamera :
    virtual public Camera {
public:
    explicit RotativeCamera(
        const glm::float32& nearpln = Camera::defaultNearPlane,
        const glm::float32& farpln = Camera::defaultFarPlane,
        const glm::vec3& head = Camera::HeadUp,
        const glm::float32& horizAngle = 0,
        const glm::float32& vertAngle = 0) noexcept;

    RotativeCamera(const RotativeCamera&) = delete;
    RotativeCamera& operator=(const RotativeCamera&) = delete;
    ~RotativeCamera() override = default;

    virtual void setHorizontalAngle(const glm::float32& horizAngle) noexcept;

    glm::float32 getHorizontalAngle() const noexcept override;

    virtual void setVerticalAngle(const glm::float32& vertAngle) noexcept;

    glm::float32 getVerticalAngle() const noexcept override;

    void applyHorizontalRotation(glm::float32 amount) noexcept;

    void applyVerticalRotation(glm::float32 amount) noexcept;

private:
    glm::float32 horizontalAngle;

    glm::float32 verticalAngle;
};
