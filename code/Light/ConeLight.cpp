#include "ConeLight.hpp"

ConeLight::ConeLight(
    const glm::vec3& color,
    glm::float32 intensity,
    const glm::vec3& position,
    const glm::vec3& direction,
    glm::float32 angle_radians,
    glm::float32 z_near,
    glm::float32 z_far
) noexcept :
    Light(color, intensity),
    m_position(position),
    m_direction(glm::normalize(direction)),
    m_angle(angle_radians),
    m_znear(z_near),
    m_zfar(z_far)
{

}

void ConeLight::setDirection(const glm::vec3& direction) noexcept {
    m_direction = direction;
}

glm::vec3 ConeLight::getDirection(void) const noexcept {
    return glm::normalize(m_direction);
}

void ConeLight::setPosition(const glm::vec3& position) noexcept {
    m_position = position;
}


glm::vec3 ConeLight::getPosition(void) const noexcept {
    return m_position;
}


void ConeLight::setAngleRadians(glm::float32 radians) noexcept {
    m_angle = radians;
}

glm::float32 ConeLight::getAngleRadians() const noexcept {
    return m_angle;
}

void ConeLight::setZNear(glm::float32 znear) noexcept {
    m_znear = znear;
}

glm::float32 ConeLight::getZNear() const noexcept {
    return m_znear;
}

void ConeLight::setZFar(glm::float32 zfar) noexcept {
    m_zfar = zfar;
}

glm::float32 ConeLight::getZFar() const noexcept {
    return m_zfar;
}

