#pragma once

#include "Texture.hpp"

#include <vector>
#include <optional>
#include <tuple>

/*
 * WARNING: Some ES drivers do not support RGB32F backing texture
 */
enum class FramebufferColorFormat {
    FRAMEBUFFER_COLOR_FORMAT_R8,
    FRAMEBUFFER_COLOR_FORMAT_RG8,
    FRAMEBUFFER_COLOR_FORMAT_RGB8,
    FRAMEBUFFER_COLOR_FORMAT_RGBA8,
    FRAMEBUFFER_COLOR_FORMAT_R32F,
    FRAMEBUFFER_COLOR_FORMAT_RG32F,
    FRAMEBUFFER_COLOR_FORMAT_RGB32F,
    FRAMEBUFFER_COLOR_FORMAT_RGBA32F,
};

class Framebuffer {
public:
    Framebuffer() = delete;
    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    virtual ~Framebuffer();

    inline GLsizei getWidth() const noexcept {
        return m_width;
    }

    inline GLsizei getHeight() const noexcept {
        return m_height;
    }

    inline const GLuint& getFramebuffer() const noexcept {
        return m_fbo;
    }

    static Framebuffer* CreateFramebuffer(
        GLsizei width,
        GLsizei height,
        const std::vector<FramebufferColorFormat>& color,
        bool depth,
        bool stencil
    ) noexcept;

    inline bool hasDepthStencilAttachment() const noexcept {
        return m_depth_stencil_attachment.has_value();
    }

    inline bool hasColorAttachment() const noexcept {
        return !m_color_attachment.empty();
    }

    inline size_t getColorAttachmentCount() const noexcept {
        return m_color_attachment.size();
    }

    inline GLuint getColorAttachment(size_t index) const noexcept {
        return m_color_attachment.at(index);
    }

    // Return the texture id of the depth (or depth-stencil) attachment, or 0
    // if no such attachment exists.
    inline GLuint getDepthAttachment() const noexcept {
        return m_depth_stencil_attachment.has_value() ? m_depth_stencil_attachment.value() : 0u;
    }

    std::tuple<const GLenum*, size_t> getAttachments(void) const noexcept {
        return std::make_tuple(m_attachments.data(), m_attachments.size());
    }

protected:
    Framebuffer(
        GLuint fbo,
        GLsizei width,
        GLsizei height,
        std::vector<GLuint>&& color_attachments,
        std::optional<GLuint>&& depth_stencil_attachment
    ) noexcept;

private:
    GLuint m_fbo;

    GLsizei m_width, m_height;

    std::vector<GLuint> m_color_attachment;
    std::optional<GLuint> m_depth_stencil_attachment;

    std::vector<GLenum> m_attachments;
};
