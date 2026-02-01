#include "Texture.hpp"

#include <iostream>

Texture::Texture(
    GLuint textureId,
    GLsizei width,
    GLsizei height
) noexcept : m_textureId(textureId), m_width(width), m_height(height) {}

Texture::~Texture() noexcept {
    Texture::DeleteTexture(m_textureId);
}

GLuint Texture::CreateTexture() noexcept {
    GLuint textureId = 0;
    glGenTextures(1, &textureId);
    return textureId;
}

void Texture::DeleteTexture(GLuint texture) noexcept {
    if (texture != 0) {
        glDeleteTextures(1, &texture);
    }
}

static uint64_t bc7_bytes(uint32_t width, uint32_t height) {
    uint64_t blocks_x = (width + 3u) / 4u;   /* ceil(width / 4) */
    uint64_t blocks_y = (height + 3u) / 4u;  /* ceil(height / 4) */
    return blocks_x * blocks_y * 16u;        /* 16 bytes per 4x4 block */
}

Texture* Texture::CreateBC7Texture2D(
    GLenum internalFormat,
    GLsizei width,
    GLsizei height,
    GLsizei mip_levels,
    const void *data
) noexcept {
    GLuint textureId = Texture::CreateTexture();
    glBindTexture(GL_TEXTURE_2D, textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (mip_levels > 1) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    if (mip_levels <= 1) {
        glCompressedTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, static_cast<GLsizei>(bc7_bytes(width, height)), data);
    
        std::cout << "Created BC7 texture " << textureId << " (" << width << "x" << height << ", no mipmaps)" << std::endl;
    } else {
        uint64_t offset = 0;
        for (GLsizei level = 0; level < mip_levels; ++level) {
            GLsizei w = width >> level;
            GLsizei h = height >> level;
            if (w < 1) w = 1;
            if (h < 1) h = 1;

            const GLsizei size = static_cast<GLsizei>(bc7_bytes(static_cast<uint32_t>(w), static_cast<uint32_t>(h)));
            const void* ptr = static_cast<const uint8_t*>(data) + offset;

            glCompressedTexImage2D(GL_TEXTURE_2D, level, internalFormat, w, h, 0, size, ptr);

            offset += static_cast<uint64_t>(size);
        }

        std::cout << "Created BC7 texture " << textureId << " (" << width << "x" << height << ", " << mip_levels << " mipmap levels)" << std::endl;
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    return new Texture(textureId, width, height);

create_compressed_error:
    Texture::DeleteTexture(textureId);

    return nullptr;
}

Texture* Texture::Create2DTexture(
    GLsizei width,
    GLsizei height,
    TextureFormat format,
    TextureDataType dataType,
    const void* data,
    TextureWrapMode wrap_s,
    TextureWrapMode wrap_t,
    TextureFilterMode min_filter,
    TextureFilterMode mag_filter
) noexcept {
    GLuint textureId = Texture::CreateTexture();
    glBindTexture(GL_TEXTURE_2D, textureId);

    GLenum internal = GL_RGBA8;
    GLenum fmt = GL_RGBA;
    GLenum type = GL_UNSIGNED_BYTE;

    switch (format) {
        case TextureFormat::TEXTURE_FORMAT_R32F:
            internal = GL_R32F;
            fmt = GL_RED;
            break;
        case TextureFormat::TEXTURE_FORMAT_R32UI:
            internal = GL_R32UI;
            fmt = GL_RED_INTEGER;
            type = GL_UNSIGNED_INT;
            break;
        case TextureFormat::TEXTURE_FORMAT_RGBA32F:
            internal = GL_RGBA32F;
            fmt = GL_RGBA;
            type = GL_FLOAT;
            break;
        case TextureFormat::TEXTURE_FORMAT_RGBA8:
            internal = GL_RGBA8;
            fmt = GL_RGBA;
            type = GL_UNSIGNED_BYTE;
            break;
        case TextureFormat::TEXTURE_FORMAT_RGB8:
        default:
            internal = GL_RGB8;
            fmt = GL_RGB;
            type = GL_UNSIGNED_BYTE;
            break;
    }

    if (dataType == TextureDataType::TEXTURE_DATA_TYPE_FLOAT) {
        type = GL_FLOAT;
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (wrap_s == TextureWrapMode::TEXTURE_WRAP_MODE_REPEAT) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (wrap_t == TextureWrapMode::TEXTURE_WRAP_MODE_REPEAT) ? GL_REPEAT : GL_CLAMP_TO_EDGE);

    GLenum minf = (min_filter == TextureFilterMode::TEXTURE_FILTER_MODE_NEAREST) ? GL_NEAREST : GL_LINEAR;
    GLenum magf = (mag_filter == TextureFilterMode::TEXTURE_FILTER_MODE_NEAREST) ? GL_NEAREST : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minf);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magf);

    // allocate and upload
    glTexImage2D(GL_TEXTURE_2D, 0, internal, width, height, 0, fmt, type, data);

    glBindTexture(GL_TEXTURE_2D, 0);

    return new Texture(textureId, width, height);
}
