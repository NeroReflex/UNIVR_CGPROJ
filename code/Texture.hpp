#pragma once

#include "OpenGL.hpp"

enum class TextureFormat {
    TEXTURE_FORMAT_RGBA8,
    TEXTURE_FORMAT_RGB8,
    TEXTURE_FORMAT_RGBA32F,
    TEXTURE_FORMAT_R32F,
    TEXTURE_FORMAT_R32UI
};

enum class TextureDataType {
    TEXTURE_DATA_TYPE_UNSIGNED_BYTE,
    TEXTURE_DATA_TYPE_FLOAT
    , TEXTURE_DATA_TYPE_UNSIGNED_INT
};

enum class TextureWrapMode {
    TEXTURE_WRAP_MODE_REPEAT,
    TEXTURE_WRAP_MODE_CLAMP_TO_EDGE
};

enum class TextureFilterMode {
    TEXTURE_FILTER_MODE_NEAREST,
    TEXTURE_FILTER_MODE_LINEAR
};

class Program;

class Texture {

    friend class Program;

    public:
        Texture() = delete;
        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        virtual ~Texture() noexcept;

        inline GLuint getTextureId() const noexcept { return m_textureId; }

        static Texture* CreateBC7Texture2D(
            GLenum internalFormat,
            GLsizei width,
            GLsizei height,
            GLsizei mip_levels,
            const void *data
        ) noexcept;

        static Texture* Create2DTexture(
            GLsizei width,
            GLsizei height,
            TextureFormat format,
            TextureDataType dataType,
            const void* data,
            TextureWrapMode wrap_s,
            TextureWrapMode wrap_t,
            TextureFilterMode min_filter,
            TextureFilterMode mag_filter
        ) noexcept;
        

        inline GLsizei getWidth() const noexcept { return m_width; }
        inline GLsizei getHeight() const noexcept { return m_height; }

    protected:
        Texture(
            GLuint textureId,
            GLsizei width,
            GLsizei height
        ) noexcept;

    private:
        static GLuint CreateTexture() noexcept;
        static void DeleteTexture(GLuint texture) noexcept;

        const GLuint m_textureId;
        GLsizei m_width, m_height;
};
