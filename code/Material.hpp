#pragma once

#include <glm/glm.hpp>

#include <memory>
#include <array>

#include "Texture.hpp"

class Material {
public:
    Material() = delete;
    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;

    Material(
        const glm::vec3& diffuse_color,
        const glm::vec3& specular_color,
        float shininess
    ) noexcept;

    inline void setDiffuseTexture(std::shared_ptr<Texture> texture) noexcept {
        m_diffuse_texture = texture;
    }

    inline std::shared_ptr<Texture> getDiffuseTexture() const noexcept {
        return m_diffuse_texture;
    }

    inline void setSpecularTexture(std::shared_ptr<Texture> texture) noexcept {
        m_specular_texture = texture;
    }

    inline std::shared_ptr<Texture> getSpecularTexture() const noexcept {
        return m_specular_texture;
    }

    inline void setDisplacementTexture(std::shared_ptr<Texture> texture) noexcept {
        m_displacement_texture = texture;
    }

    inline std::shared_ptr<Texture> getDisplacementTexture() const noexcept {
        return m_displacement_texture;
    }

    void bindRenderState(
        GLuint diffuse_color_location,
        GLuint specular_color_location,
        GLuint material_uniform_location,
        GLint shininess_location
    ) const noexcept;

private:
    glm::vec3 m_diffuse_color;

    glm::vec3 m_specular_color;

    float m_shininess;

    std::shared_ptr<Texture> m_diffuse_texture;

    std::shared_ptr<Texture> m_specular_texture;

    std::shared_ptr<Texture> m_displacement_texture;
};
