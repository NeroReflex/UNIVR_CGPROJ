
#include "ConeLight.hpp"

ConeLight::ConeLight(
    const glm::vec3& color,
    glm::float32 intensity,
    const glm::vec3& position,
    const glm::vec3& direction,
    glm::float32 angle_radians,
    glm::float32 z_near,
    glm::float32 z_far,
    glm::float32 inner_ratio,
    glm::float32 constant,
    glm::float32 linear,
    glm::float32 quadratic
) noexcept :
    Light(color, intensity),
    m_position(position),
    m_direction(glm::normalize(direction)),
    m_angle(angle_radians),
    m_znear(z_near),
    m_zfar(z_far),
    m_inner_ratio(inner_ratio),
    m_constant(constant),
    m_linear(linear),
    m_quadratic(quadratic)
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

void ConeLight::setInnerRatio(glm::float32 r) noexcept {
    m_inner_ratio = r;
}

glm::float32 ConeLight::getInnerRatio() const noexcept {
    return m_inner_ratio;
}

void ConeLight::setConstant(glm::float32 c) noexcept {
    m_constant = c;
}

glm::float32 ConeLight::getConstant() const noexcept {
    return m_constant;
}

void ConeLight::setLinear(glm::float32 l) noexcept {
    m_linear = l;
}

glm::float32 ConeLight::getLinear() const noexcept {
    return m_linear;
}

void ConeLight::setQuadratic(glm::float32 q) noexcept {
    m_quadratic = q;
}

glm::float32 ConeLight::getQuadratic() const noexcept {
    return m_quadratic;
}

