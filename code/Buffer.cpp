#include "Buffer.hpp"

Buffer::Buffer(
    GLuint sbo,
    GLsizei size
) noexcept :
m_sbo(sbo),
m_size(size)
{
    // constructor body left empty
}

Buffer::~Buffer()
{
    if (m_sbo != 0) {
        CHECK_GL_ERROR(glDeleteBuffers(1, &m_sbo));
        m_sbo = 0;
    }
}

Buffer* Buffer::CreateBuffer(
    const void *const data,
    GLsizei size
) noexcept
{
    GLuint sbo;
    CHECK_GL_ERROR(glGenBuffers(1, &sbo));
    CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, sbo));
    CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));

    return new Buffer(sbo, size);
}