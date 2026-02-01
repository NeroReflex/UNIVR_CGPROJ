#pragma once

#include "OpenGL.hpp"

class Program;

class Buffer {

    friend class Program;

public:
    Buffer() = delete;
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    virtual ~Buffer();

    inline GLsizei getSize() const noexcept {
        return m_size;
    }

    static Buffer* CreateBuffer(
        const void *const data,
        GLsizei size
    ) noexcept;

protected:
    Buffer(
        GLuint sbo,
        GLsizei size
    ) noexcept;

private:
    GLuint m_sbo;

    GLsizei m_size;
};
