#pragma once

#include "Camera.hpp"

class MovableCamera :
    //virtual public Movable,
    virtual public Camera {
public:
    explicit MovableCamera(
        const glm::vec3& position,
        const glm::float32& nearpln = Camera::defaultNearPlane,
        const glm::float32& farpln = Camera::defaultFarPlane,
        const glm::vec3& head = Camera::HeadUp) noexcept;

    MovableCamera(const MovableCamera&) = delete;
    MovableCamera& operator=(const MovableCamera&) = delete;
    ~MovableCamera() override = default;

    glm::vec3 getCameraPosition() const noexcept final;

    void setCameraPosition(const glm::vec3& position) noexcept;

    void applyMovement(const glm::vec3& moveVect, glm::float32 amount) noexcept;

private:
    glm::vec3 position;
};
