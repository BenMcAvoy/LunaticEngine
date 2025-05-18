#include "pch.h"

#include "shader.h"

using namespace Lunatic;

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) {
	std::string vertexSrc = loadShaderSource(vertexPath);
	std::string fragmentSrc = loadShaderSource(fragmentPath);
	unsigned int vertex = compileShader(GL_VERTEX_SHADER, vertexSrc.c_str());
	unsigned int fragment = compileShader(GL_FRAGMENT_SHADER, fragmentSrc.c_str());
	m_id = createProgram(vertex, fragment);
}

Shader::Shader() {
    unsigned int vertex = compileShader(GL_VERTEX_SHADER, DEFAULT_VERTEX_SRC);
    unsigned int fragment = compileShader(GL_FRAGMENT_SHADER, DEFAULT_FRAGMENT_SRC);
    m_id = createProgram(vertex, fragment);
}

std::string Shader::loadShaderSource(const std::string& path) const {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Shader::loadShaderSource - Failed to open file: " + path);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

unsigned int Shader::compileShader(unsigned int type, const char* source) const {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success = 0;
    char infoLog[512] = {};
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
        std::string typeStr = (type == GL_VERTEX_SHADER) ? "Vertex" : "Fragment";
        throw std::runtime_error("Shader::compileShader - Failed to compile " + typeStr + " shader:\n" + infoLog);
    }

	spdlog::info("Shader::compileShader - Compiled {} shader successfully", type == GL_VERTEX_SHADER ? "vertex" : "fragment");

    return shader;
}

unsigned int Shader::createProgram(unsigned int vertex, unsigned int fragment) const {
    unsigned int program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    int success = 0;
    char infoLog[512] = {};
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
        throw std::runtime_error("Shader::createProgram - Failed to link program:\n" + std::string(infoLog));
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return program;
}

Shader::~Shader() {
  glDeleteProgram(m_id);
}

void Shader::use() const { glUseProgram(m_id); }

void Shader::set(const std::string_view name, float value) const {
    GLint loc = glGetUniformLocation(m_id, name.data());
    if (loc == -1) throw std::runtime_error("Uniform '" + std::string(name) + "' not found");
    glUniform1f(loc, value);
}

void Shader::set(const std::string_view name, int value) const {
    GLint loc = glGetUniformLocation(m_id, name.data());
    if (loc == -1) throw std::runtime_error("Uniform '" + std::string(name) + "' not found");
    glUniform1i(loc, value);
}

void Shader::set(const std::string_view name, bool value) const {
    GLint loc = glGetUniformLocation(m_id, name.data());
    if (loc == -1) throw std::runtime_error("Uniform '" + std::string(name) + "' not found");
    glUniform1i(loc, static_cast<int>(value));
}

void Shader::set(const std::string_view name, GLfloat* value) const {
    GLint loc = glGetUniformLocation(m_id, name.data());
    if (loc == -1) throw std::runtime_error("Uniform '" + std::string(name) + "' not found");
    glUniformMatrix4fv(loc, 1, GL_FALSE, value);
}

void Shader::set(const std::string_view name, const glm::mat4& value) const {
    GLint loc = glGetUniformLocation(m_id, name.data());
    if (loc == -1) throw std::runtime_error("Uniform '" + std::string(name) + "' not found");
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::set(const std::string_view name, const glm::vec3& value) const {
    GLint loc = glGetUniformLocation(m_id, name.data());
    if (loc == -1) throw std::runtime_error("Uniform '" + std::string(name) + "' not found");
    glUniform3fv(loc, 1, glm::value_ptr(value));
}

void Shader::set(const std::string_view name, float* value, int count) const {
    GLint loc = glGetUniformLocation(m_id, name.data());
    if (loc == -1) throw std::runtime_error("Uniform '" + std::string(name) + "' not found");

    switch (count) {
    case 1:
        glUniform1fv(loc, 1, value);
        break;
    case 2:
        glUniform2fv(loc, 1, value);
        break;
    case 3:
        glUniform3fv(loc, 1, value);
        break;
    case 4:
        glUniform4fv(loc, 1, value);
        break;
    default:
        throw std::invalid_argument("Shader::set(): Invalid float count: " + std::to_string(count));
    }
}
