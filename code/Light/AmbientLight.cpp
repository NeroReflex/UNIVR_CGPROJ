#include "AmbientLight.hpp"

AmbientLight::AmbientLight(
    const glm::vec3& color,
    glm::float32 intensity
) noexcept :
    Light(color, intensity)
{

}
