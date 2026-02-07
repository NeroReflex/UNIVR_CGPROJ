#pragma once

#include "Shader.hpp"
#include "Texture.hpp"
#include "Framebuffer.hpp"
#include "Buffer.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <memory>
#include <unordered_map>
#include <string>

class Program {
public:
    Program() = delete;
    Program(const Program&) = delete;
    Program& operator=(const Program&) = delete;

    ~Program();

    Program(
        GLuint program,
        std::unordered_map<std::string, GLint>&& uniform_locations,
        std::unordered_map<std::string, GLint>&& storage_locations,
        std::unordered_map<std::string, GLint>&& attribute_locations
    ) noexcept;

    static Program* LinkProgram(
        const VertexShader *const vertShader,
        const GeometryShader *const geomShader,
        const FragmentShader *const fragShader
    ) noexcept;

    static Program* LinkProgram(
        const ComputeShader *const compShader
    ) noexcept;

    inline const GLuint& getProgram() const noexcept {
        return m_program;
    }

    void uniformMat4x4(
        const std::string& name,
        const glm::mat4& val
    ) noexcept;

    void uniformMat3x3(
        const std::string& name,
        const glm::mat3& val
    ) noexcept;

    void uniformVec3(
        const std::string& name,
        const glm::vec3& val
    ) noexcept;

    void uniformVec4(
        const std::string& name,
        const glm::vec4& val
    ) noexcept;

    void uniformFloat(
        const std::string& name,
        const glm::float32& val
    ) noexcept;

    void uniformUint(
        const std::string& name,
        const glm::uint32& val
    ) noexcept;

    void uniformInt(
        const std::string& name,
        const glm::int32& val
    ) noexcept;

    void texture(
        const std::string& name,
        GLuint texture_unit,
        const Texture& texture
    ) noexcept;

    void framebufferColorAttachment(
        const std::string& name,
        GLuint texture_unit,
        const Framebuffer& framebuffer,
        size_t index
    ) noexcept;

    void framebufferDepthAttachment(
        const std::string& name,
        GLuint texture_unit,
        const Framebuffer& framebuffer
    ) noexcept;

    void bind() const noexcept;

    void uniformStorageBuffer(
        const std::string& name,
        const Buffer& buf
    ) noexcept;

    void uniformStorageBufferBinding(
        const std::string& name,
        GLuint bufferId
    ) noexcept;

    void dispatchCompute(
        GLuint num_groups_x,
        GLuint num_groups_y,
        GLuint num_groups_z
    ) noexcept;

private:
    void framebufferAttachment(
        const std::string& name,
        GLuint texture_unit,
        GLuint attachment
    ) noexcept;

    GLint getUniformLocation(const std::string& name) noexcept;

    GLint getStorageLocation(const std::string& name) noexcept;

    GLint getAttributeLocation(const std::string& name) noexcept;

    GLuint m_program;

    std::unordered_map<std::string, GLint> m_uniform_locations;

    std::unordered_map<std::string, GLint> m_storage_locations;

    std::unordered_map<std::string, GLint> m_attrib_locations;
};
