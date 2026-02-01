#include "OrthoSpectatorCamera.hpp"

OrthoSpectatorCamera::OrthoSpectatorCamera(
    const glm::vec3& initialPosition,
    const glm::float32& nearpln,
    const glm::float32& farpln,
    const glm::vec3& head,
    const glm::float32& orthoHeight,
    const glm::float32& horizAngle,
    const glm::float32& vertAngle
) noexcept
    : Camera(nearpln, farpln, head),
    RotativeCamera(nearpln, farpln, head, horizAngle, vertAngle),
    OrthoCamera(nearpln, farpln, head, orthoHeight),
    MovableCamera(initialPosition, nearpln, farpln, head) {}
