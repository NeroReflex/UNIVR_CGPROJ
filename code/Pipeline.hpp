#pragma once

#include "Program.hpp"
#include "Framebuffer.hpp"

#include <functional>

class Scene;

class Pipeline {
public:
    Pipeline() = delete;
    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;

    virtual ~Pipeline();

    virtual void render(const Scene *const scene) noexcept = 0;

    /**
     * Resize the pipeline's internal resources to the new width and height.
     * 
     * A class that inherits from Pipeline should override this method and
     * call it to update the stored width and height at the end (if everything went well).
     * 
     * @param width New width in pixels.
     * @param height New height in pixels.
     */
    virtual bool resize(GLsizei width, GLsizei height) noexcept;

protected:
    Pipeline(GLsizei width, GLsizei height) noexcept;

    inline void bindDefaultFramebuffer() const noexcept {
        CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    }

    void withEnabledDepthTest(std::function<void()> fn) const noexcept {
        GLboolean prevDepthMask = GL_TRUE;
        CHECK_GL_ERROR(glGetBooleanv(GL_DEPTH_WRITEMASK, &prevDepthMask));

        CHECK_GL_ERROR(glEnable(GL_DEPTH_TEST));
        CHECK_GL_ERROR(glDepthFunc(GL_LESS));
        CHECK_GL_ERROR(glDepthMask(GL_TRUE));

        fn();

        // restore previous depth write mask and disable depth test
        CHECK_GL_ERROR(glDepthMask(prevDepthMask));
        CHECK_GL_ERROR(glDisable(GL_DEPTH_TEST));
    }

    inline void withFramebuffer(const Framebuffer *const fbo, std::function<void()> fn) const noexcept {
        GLint prev_fbo = 0;
        CHECK_GL_ERROR(glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo));

        GLint prev_viewport[4] = {0, 0, 0, 0};
        CHECK_GL_ERROR(glGetIntegerv(GL_VIEWPORT, prev_viewport));

        CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, fbo->getFramebuffer()));

        // set viewport to match the framebuffer before clearing so the
        // clear covers the whole attachment texture.
        bindViewport(0, 0, fbo->getWidth(), fbo->getHeight());

        // ensure draw buffers point to fbo attachments while the FBO is bound
        const auto [attachments_data, attachments_count] = fbo->getAttachments();
        if (attachments_count > 0) {
            CHECK_GL_ERROR(glDrawBuffers(attachments_count, attachments_data));
        } else {
            CHECK_GL_ERROR(glDrawBuffers(0, nullptr));
        }

        GLbitfield clear_flags = 0;
        if (fbo->hasColorAttachment())
            clear_flags |= GL_COLOR_BUFFER_BIT;

        if (fbo->hasDepthStencilAttachment())
            clear_flags |= GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;

        CHECK_GL_ERROR(glClear(clear_flags));

        fn();

        // restore previous framebuffer binding and viewport
        CHECK_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prev_fbo)));
        bindViewport(prev_viewport[0], prev_viewport[1], prev_viewport[2], prev_viewport[3]);
    }

    inline void withFaceCulling(std::function<void()> fn) const noexcept {
        CHECK_GL_ERROR(glEnable(GL_CULL_FACE));
        CHECK_GL_ERROR(glCullFace(GL_BACK));
        CHECK_GL_ERROR(glFrontFace(GL_CCW));

        fn();

        CHECK_GL_ERROR(glDisable(GL_CULL_FACE));
    }

    void bindViewport(GLint x, GLint y, GLsizei width, GLsizei height) const noexcept {
        CHECK_GL_ERROR(glViewport(x, y, width, height));
    }

private:
    GLsizei m_width = 0;
    GLsizei m_height = 0;
};
