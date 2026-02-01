#include "Light.hpp"

Light::Light(const glm::vec3& color, glm::float32 intensity) noexcept :
    m_color(color),
    m_intensity(intensity)
{

}

void Light::setColor(const glm::vec3& color) noexcept {
    m_color = color;
}

glm::vec3 Light::getColor(void) const noexcept {
    return m_color;
}

void Light::setIntensity(glm::float32 intensity) noexcept {
    m_intensity = intensity;
}

glm::float32 Light::getIntensity() const noexcept {
    return m_intensity;
}

glm::vec3 Light::getColorWithIntensity(void) const noexcept {
    return m_color * m_intensity;
}
