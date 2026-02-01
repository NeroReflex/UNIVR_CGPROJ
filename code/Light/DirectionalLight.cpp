#include "DirectionalLight.hpp"

DirectionalLight::DirectionalLight(
    const glm::vec3& color,
    glm::float32 intensity,
    const glm::vec3& direction
) noexcept :
    Light(color, intensity),
    m_direction(glm::normalize(direction))
{

}

void DirectionalLight::setDirection(const glm::vec3& direction) noexcept {
    m_direction = direction;
}

glm::vec3 DirectionalLight::getDirection(void) const noexcept {
    return glm::normalize(m_direction);
}

