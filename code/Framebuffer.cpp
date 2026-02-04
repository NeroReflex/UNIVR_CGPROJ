#include "Framebuffer.hpp"

#include <cstdio>
#include <cassert>

static std::tuple<GLint, GLenum, GLenum> get_native_format(const FramebufferColorFormat& format) noexcept {
    switch (format) {
        case FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_R8:
            return { GL_R8, GL_RED, GL_UNSIGNED_BYTE };
        case FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_RG8:
            return { GL_RG8, GL_RG, GL_UNSIGNED_BYTE };
        case FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_RGB8:
            return { GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE };
        case FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_RGBA8:
            return { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE };
        case FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_R32F:
            return { GL_R32F, GL_RED, GL_FLOAT };
        case FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_RG32F:
            return { GL_RG32F, GL_RG, GL_FLOAT };
        case FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_RGB32F:
            return { GL_RGB32F, GL_RGB,  GL_FLOAT };
        case FramebufferColorFormat::FRAMEBUFFER_COLOR_FORMAT_RGBA32F:
            return { GL_RGBA32F, GL_RGBA,  GL_FLOAT };
        default:
            assert(1 == 0 && "Unknown framebuffer color format");
    }

    // solve a warning about reacing end of non-void function
    return { GL_R8, GL_RED, GL_UNSIGNED_BYTE };
}

Framebuffer::Framebuffer(
    GLuint fbo,
    GLsizei width,
    GLsizei height,
    std::vector<GLuint>&& color_attachments,
    std::optional<GLuint>&& depth_stencil_attachment
) noexcept :
    m_fbo(fbo),
    m_width(width),
    m_height(height),
    m_color_attachment(std::move(color_attachments)),
    m_depth_stencil_attachment(std::move(depth_stencil_attachment))
{
    for (size_t i = 0; i < getColorAttachmentCount(); ++i) {
        m_attachments.push_back(GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i));
    }
}

Framebuffer::~Framebuffer() {
    for (const auto& color_attachment : m_color_attachment) {
        glDeleteTextures(1, &color_attachment);
    }

    if (m_depth_stencil_attachment.has_value()) {
        const GLuint depth_attachment = m_depth_stencil_attachment.value();
        // depth/stencil was created as a texture so delete it as such
        glDeleteTextures(1, &depth_attachment);
    }

    if (m_fbo != 0) {
        glDeleteFramebuffers(1, &m_fbo);
    }
}

Framebuffer* Framebuffer::CreateFramebuffer(
    GLsizei width,
    GLsizei height,
    const std::vector<FramebufferColorFormat>& color,
    bool depth,
    bool stencil
) noexcept {
    Framebuffer* framebuffer = nullptr;

    GLuint fbo = 0;

    std::vector<GLuint> color_attachments;
    std::optional<GLuint> depth_stencil_attachment;

    color_attachments.reserve(color.size());

    CHECK_GL_ERROR(glGenFramebuffers(1, &fbo));
    CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, fbo));
    if (fbo == 0) {
        printf("Failed to create framebuffer object\n");
        goto create_fbo_error;
    }

    for (GLsizei i = 0; i < static_cast<GLsizei>(color.size()); ++i) {
        GLuint texture = 0;
        CHECK_GL_ERROR(glGenTextures(1, &texture));
        if (texture == 0) {
            printf("Failed to create texture for framebuffer color attachment %d\n", i);
            goto create_texture_error;
        }

        CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_2D, texture));

        const auto [internalFormat, format, type] = get_native_format(color[i]);

        CHECK_GL_ERROR(glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, NULL));

        CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

        const GLenum attachment = GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i);
        CHECK_GL_ERROR(glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture, 0));

        color_attachments.push_back(texture);
    }

    if (depth) {
        // Create a depth-stencil (or depth only) texture so we can sample depth in lighting passes
        GLuint depth_tex = 0;
        CHECK_GL_ERROR(glGenTextures(1, &depth_tex));
        if (depth_tex == 0) {
            printf("Failed to create depth texture for framebuffer depth attachment\n");
            goto create_depth_stencil_error;
        }

        depth_stencil_attachment.emplace(depth_tex);

        CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_2D, depth_tex));

        if (stencil) {
            // Create combined depth-stencil texture
            CHECK_GL_ERROR(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL));
            CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
            CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
            CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
            CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
            CHECK_GL_ERROR(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depth_tex, 0));
        } else {
            // Create depth-only texture (better for sampling as a sampler2D in shaders)
            CHECK_GL_ERROR(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL));
            CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
            CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
            CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
            CHECK_GL_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
            CHECK_GL_ERROR(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_tex, 0));
        }
    } else if (stencil) {
        assert(1 == 0 && "Stencil-only framebuffer attachments are not supported");
    }

    // Use the least amount of memory needed
    color_attachments.shrink_to_fit();

    // Set draw buffers to match created color attachments. If there are no
    // color attachments (depth-only framebuffer) explicitly disable color
    // outputs by setting draw/read buffers to GL_NONE.
    if (!color_attachments.empty()) {
        std::vector<GLenum> draw_buffers;
        draw_buffers.reserve(color_attachments.size());
        for (GLsizei i = 0; i < static_cast<GLsizei>(color_attachments.size()); ++i) {
            draw_buffers.push_back(GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i));
        }
        CHECK_GL_ERROR(glDrawBuffers(static_cast<GLsizei>(draw_buffers.size()), draw_buffers.data()));
    } else {
        // Depth-only framebuffer: ensure no color buffer is written. Use
        // glDrawBuffers with a single GL_NONE entry which is available on
        // GLES3 / desktop via glad. Avoid glReadBuffer as it may be unavailable.
        const GLenum none = GL_NONE;
        CHECK_GL_ERROR(glDrawBuffers(1, &none));
    }

    // Verify framebuffer completeness
    {
        const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            printf("Failed to create framebuffer: status=0x%04X\n", status);
            goto create_texture_error;
        }
    }

    framebuffer = new Framebuffer(fbo, width, height, std::move(color_attachments), std::move(depth_stencil_attachment));
    if (framebuffer == nullptr) {
        printf("Failed to create Framebuffer instance: out-of-memory\n");
        goto create_depth_stencil_error;
    }

    return framebuffer;

create_depth_stencil_error:
    if (depth_stencil_attachment.has_value()) {
        const GLuint depth_tex = depth_stencil_attachment.value();
        glDeleteTextures(1, &depth_tex);
    }

create_texture_error:
    for (const auto& color_attachment : color_attachments) {
        glDeleteTextures(1, &color_attachment);
    }
    color_attachments.clear();

create_fbo_error:
    if (fbo != 0) {
        glDeleteFramebuffers(1, &fbo);
    }

    return framebuffer;
}
