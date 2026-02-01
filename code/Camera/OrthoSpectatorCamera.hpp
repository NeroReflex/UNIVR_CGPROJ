#pragma once

#include "OrthoCamera.hpp"
#include "MovableCamera.hpp"
#include "RotativeCamera.hpp"

class OrthoSpectatorCamera final :
    virtual public MovableCamera,
    virtual public RotativeCamera,
    virtual public OrthoCamera {
public:
    explicit OrthoSpectatorCamera(
        const glm::vec3& initialPosition,
        const glm::float32& nearpln = Camera::defaultNearPlane,
        const glm::float32& farpln = Camera::defaultFarPlane,
        const glm::vec3& head = Camera::HeadUp,
        const glm::float32& orthoHeight = OrthoCamera::defaultOrthoHeight,
        const glm::float32& horizAngle = 0,
        const glm::float32& vertAngle = 0) noexcept;

    OrthoSpectatorCamera() = delete;
    OrthoSpectatorCamera(const OrthoSpectatorCamera&) = delete;
    OrthoSpectatorCamera& operator=(const OrthoSpectatorCamera&) = delete;
    ~OrthoSpectatorCamera() override = default;

    using MovableCamera::getCameraPosition;
};
