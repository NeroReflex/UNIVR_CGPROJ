#pragma once

#include "ProjectionCamera.hpp"
#include "MovableCamera.hpp"
#include "RotativeCamera.hpp"

class SpectatorCamera final :
    //virtual public Game::Controllable,
    virtual public MovableCamera,
    virtual public RotativeCamera,
    virtual public ProjectionCamera {
public:
    explicit SpectatorCamera(
        const glm::vec3& initialPosition,
        const glm::float32& nearpln = Camera::defaultNearPlane,
        const glm::float32& farpln = Camera::defaultFarPlane,
        const glm::vec3& head = Camera::HeadUp,
        const glm::float32& fovDegree = 60,
        const glm::float32& horizAngle = 0,
        const glm::float32& vertAngle = 0) noexcept;

    SpectatorCamera() = delete;
    SpectatorCamera(const SpectatorCamera&) = delete;
    SpectatorCamera& operator=(const SpectatorCamera&) = delete;
    ~SpectatorCamera() override = default;

    using MovableCamera::getCameraPosition;

    /*using RotativeCamera::getVerticalAngle;
    using RotativeCamera::getHorizontalAngle;*/

};
