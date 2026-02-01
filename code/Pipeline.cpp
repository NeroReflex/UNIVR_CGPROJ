#include "Pipeline.hpp"

Pipeline::Pipeline(GLsizei width, GLsizei height) noexcept :
    m_width(width),
    m_height(height)
{}

Pipeline::~Pipeline() { }

bool Pipeline::resize(GLsizei width, GLsizei height) noexcept {
    m_width = width;
    m_height = height;

    return true;
}
