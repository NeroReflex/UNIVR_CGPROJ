#include "Program.hpp"

#include <cstdio>

static void printProgramLog(GLuint prog) {
    GLint len = 0;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
    if (len > 1) {
        auto log = new char[len];
        glGetProgramInfoLog(prog, len, NULL, log);
        fprintf(stderr, "Program log:\n%s\n", log);
        delete [] log;
    }
}

Program::Program(
    GLuint program,
    std::unordered_map<std::string, GLint>&& uniform_locations,
    std::unordered_map<std::string, GLint>&& storage_locations,
    std::unordered_map<std::string, GLint>&& attribute_locations
) noexcept :
    m_program(program),
    m_uniform_locations(uniform_locations),
    m_storage_locations(storage_locations),
    m_attrib_locations(attribute_locations)
{
    // Intentionally left empty
}

Program::~Program() {
    glDeleteProgram(m_program);
}

Program* Program::LinkProgram(
    const VertexShader *const vertShader,
    const GeometryShader *const geomShader,
    const FragmentShader *const fragShader
) noexcept {
    GLuint prog = glCreateProgram();
    if (!prog) return 0;

    glAttachShader(prog, vertShader->m_shader);
    if (geomShader)
        glAttachShader(prog, geomShader->m_shader);
    glAttachShader(prog, fragShader->m_shader);

    glLinkProgram(prog);

    glDetachShader(prog, vertShader->m_shader);
    if (geomShader)
        glDetachShader(prog, geomShader->m_shader);
    glDetachShader(prog, fragShader->m_shader);

    GLint linked = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &linked);
    if (!linked) {
        fprintf(stderr, "Program failed to link.\n");
        printProgramLog(prog);
        glDeleteProgram(prog);
        return nullptr;
    }

    std::unordered_map<std::string, GLint> uniform_locations;
    std::unordered_map<std::string, GLint> storage_locations;
    std::unordered_map<std::string, GLint> attribute_locations;

    return new Program(
        prog,
        std::move(uniform_locations),
        std::move(storage_locations),
        std::move(attribute_locations)
    );
}

void Program::texture(const std::string& name, GLuint texture_unit, const Texture& texture) noexcept {
    if (texture_unit >= GL_TEXTURE0) {
        texture_unit -= GL_TEXTURE0;
    }

    CHECK_GL_ERROR(glActiveTexture(GL_TEXTURE0 + texture_unit));
    CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_2D, texture.m_textureId));
    const GLint tex_loc = getUniformLocation(name);
    assert(tex_loc >= 0);
    CHECK_GL_ERROR(glUniform1i(tex_loc, texture_unit));
    
    GLint loc = getUniformLocation(name);
    if (loc >= 0)
        CHECK_GL_ERROR(glUniform1i(loc, static_cast<GLint>(texture_unit)));
}

void Program::uniformMat4x4(const std::string& name, const glm::mat4& val) noexcept {
    GLint loc = getUniformLocation(name);
    if (loc >= 0)
        CHECK_GL_ERROR(glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(val)));
}

void Program::uniformMat3x3(const std::string& name, const glm::mat3& val) noexcept {
    GLint loc = getUniformLocation(name);
    if (loc >= 0)
        CHECK_GL_ERROR(glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(val)));
}

void Program::uniformVec3(const std::string& name, const glm::vec3& val) noexcept {
    GLint loc = getUniformLocation(name);
    if (loc >= 0)
        CHECK_GL_ERROR(glUniform3fv(loc, 1, glm::value_ptr(val)));
}

void Program::uniformVec4(const std::string& name, const glm::vec4& val) noexcept {
    GLint loc = getUniformLocation(name);
    if (loc >= 0)
        CHECK_GL_ERROR(glUniform4fv(loc, 1, glm::value_ptr(val)));
}

void Program::uniformFloat(const std::string& name, const glm::float32& val) noexcept {
    GLint loc = getUniformLocation(name);
    if (loc >= 0)
        CHECK_GL_ERROR(glUniform1f(loc, val));
}

void Program::uniformUint(const std::string& name, const glm::uint32& val) noexcept {
    GLint loc = getUniformLocation(name);
    if (loc >= 0)
        CHECK_GL_ERROR(glUniform1ui(loc, val));
}

void Program::bind() const noexcept {
    CHECK_GL_ERROR(glUseProgram(m_program));
}

GLint Program::getUniformLocation(const std::string& name) noexcept {
    // First consult cached uniform locations collected at link time.
    const auto it = m_uniform_locations.find(name);
    if (it != m_uniform_locations.end()) {
        return it->second;
    }

    // Fallback: query from driver and cache result; warn if missing.
    const auto loc = glGetUniformLocation(m_program, name.c_str());
    if (loc >= 0) {
        // Cache only valid locations. Do not cache -1 so that future
        // lookups (e.g. after relinking) can still succeed.
        m_uniform_locations[name] = loc;
    } else {
        fprintf(stderr, "Warning: uniform '%s' not found in program %u: %d\n", name.c_str(), m_program, (int)loc);
    }

    return loc;
}

GLint Program::getAttributeLocation(const std::string& name) noexcept {
    // First consult cached uniform locations collected at link time.
    const auto it = m_attrib_locations.find(name);
    if (it != m_attrib_locations.end()) {
        return it->second;
    }

    // Fallback: query from driver and cache result; warn if missing.
    const auto loc = glGetAttribLocation(m_program, name.c_str());
    if (loc >= 0) {
        // Cache only valid locations. Do not cache -1 so that future
        // lookups (e.g. after relinking) can still succeed.
        m_attrib_locations[name] = loc;
    } else {
        fprintf(stderr, "Warning: attribute '%s' not found in program %u: %d\n", name.c_str(), m_program, (int)loc);
    }

    return loc;
}

GLint Program::getStorageLocation(const std::string& name) noexcept {
    // First consult cached uniform locations collected at link time.
    const auto it = m_storage_locations.find(name);
    if (it != m_storage_locations.end()) {
        return it->second;
    }

    // Fallback: query from driver and cache result; warn if missing.
    const auto loc = glGetProgramResourceIndex(m_program, GL_SHADER_STORAGE_BLOCK, name.c_str());
    if (loc >= 0) {
        // Cache only valid locations. Do not cache -1 so that future
        // lookups (e.g. after relinking) can still succeed.
        m_storage_locations[name] = loc;
    } else {
        fprintf(stderr, "Warning: shader storage block '%s' not found in program %u\n", name.c_str(), m_program);
    }

    // Query the binding point assigned to this block (if the shader used
    // `layout(binding = N)` the driver will report that binding). If the
    // property query fails, fall back to using the block index as the
    // binding point (not ideal but better than nothing).
    GLint binding = -1;
    {
        GLenum props[] = { GL_BUFFER_BINDING };
        GLint params = 0;
        glGetProgramResourceiv(m_program, GL_SHADER_STORAGE_BLOCK, loc, 1, props, 1, &params, &binding);
    }

    if (binding < 0) {
        binding = static_cast<GLint>(loc);
    }

    return binding;
}

void Program::uniformStorageBuffer(const std::string& name, const Buffer& buf) noexcept {
    GLint loc = getStorageLocation(name);
    if (loc >= 0) {
        CHECK_GL_ERROR(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, loc, buf.m_sbo));
    }
}
