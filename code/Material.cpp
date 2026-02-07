#include "Material.hpp"

#include <glm/gtc/type_ptr.hpp>

Material::Material(
    const glm::vec3& diffuse_color,
    const glm::vec3& specular_color,
    float shininess
) noexcept :
    m_diffuse_color(diffuse_color),
    m_specular_color(specular_color),
    m_shininess(shininess),
    m_diffuse_texture(nullptr),
    m_specular_texture(nullptr),
    m_displacement_texture(nullptr)
{

}

void Material::bindRenderState(
    GLuint diffuse_color_location,
    GLuint specular_color_location,
    GLuint material_uniform_location,
    GLint shininess_location
) const noexcept {
    const auto diffuse_texture = getDiffuseTexture();

    glm::uint material_flags = 0x00000000;

    if (diffuse_texture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuse_texture->getTextureId());
    
        material_flags |= 0x00000001;
    } else {
        // ensure unit 0 has no texture bound
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    const auto specular_texture = getSpecularTexture();
    if (specular_texture) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specular_texture->getTextureId());

        material_flags |= 0x00000002;
    } else {
        // ensure unit 1 has no texture bound
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    const auto displacement_texture = getDisplacementTexture();
    if (displacement_texture) {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, displacement_texture->getTextureId());

        material_flags |= 0x00000004;
    } else {
        // ensure unit 2 has no texture bound
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if (diffuse_color_location != 0) {
        glUniform3fv(diffuse_color_location, 1, glm::value_ptr(m_diffuse_color));
    }

    if (specular_color_location != 0) {
        glUniform3fv(specular_color_location, 1, glm::value_ptr(m_specular_color));
    }

    if (material_uniform_location != 0) {
        glUniform1ui(material_uniform_location, material_flags);
    }

    if (shininess_location != 0) {
        glUniform1f(shininess_location, m_shininess);
    }
}
