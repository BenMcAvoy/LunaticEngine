#include "pch.h"

#include "shader.h"

using namespace Lunatic;

Shader::Shader(std::string_view vertexPath, std::string_view fragmentPath) {
	std::string vertexSrc = loadShaderSource(vertexPath);
	std::string fragmentSrc = loadShaderSource(fragmentPath);
	GLuint vertex = compileShader(GL_VERTEX_SHADER, vertexSrc.c_str());
	GLuint fragment = compileShader(GL_FRAGMENT_SHADER, fragmentSrc.c_str());
	programID_ = createProgram(vertex, fragment);
}

Shader::Shader() {
    GLuint vertex = compileShader(GL_VERTEX_SHADER, DEFAULT_VERTEX_SRC);
    GLuint fragment = compileShader(GL_FRAGMENT_SHADER, DEFAULT_FRAGMENT_SRC);
    programID_ = createProgram(vertex, fragment);
}

std::string Shader::loadShaderSource(std::string_view path) const {
    std::ifstream file(path.data());
    if (!file.is_open()) {
        throw std::runtime_error("Shader::loadShaderSource - Failed to open file: " + std::string(path));
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint Shader::compileShader(GLenum type, const char* source) const {
    GLuint shader = glCreateShader(type);
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

    return shader;
}

GLuint Shader::createProgram(GLuint vertex, GLuint fragment) const {
    GLuint program = glCreateProgram();
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
  glDeleteProgram(programID_);
}

void Shader::use() const { glUseProgram(programID_); }

void Shader::set(const std::string_view name, float value) const {
    GLint loc = glGetUniformLocation(programID_, name.data());
    if (loc == -1) throw std::runtime_error("Uniform '" + std::string(name) + "' not found");
    glUniform1f(loc, value);
}

void Shader::set(const std::string_view name, int value) const {
    GLint loc = glGetUniformLocation(programID_, name.data());
    if (loc == -1) throw std::runtime_error("Uniform '" + std::string(name) + "' not found");
    glUniform1i(loc, value);
}

void Shader::set(const std::string_view name, bool value) const {
    GLint loc = glGetUniformLocation(programID_, name.data());
    if (loc == -1) throw std::runtime_error("Uniform '" + std::string(name) + "' not found");
    glUniform1i(loc, static_cast<int>(value));
}

void Shader::set(const std::string_view name, GLfloat* value) const {
    GLint loc = glGetUniformLocation(programID_, name.data());
    if (loc == -1) throw std::runtime_error("Uniform '" + std::string(name) + "' not found");
    glUniformMatrix4fv(loc, 1, GL_FALSE, value);
}

void Shader::set(const std::string_view name, GLuint value) const {
    GLint loc = glGetUniformLocation(programID_, name.data());
    if (loc == -1) throw std::runtime_error("Uniform '" + std::string(name) + "' not found");
	glUniform1i(loc, static_cast<GLint>(value));
}

void Shader::set(const std::string_view name, const glm::mat4& value) const {
    GLint loc = glGetUniformLocation(programID_, name.data());
    if (loc == -1) throw std::runtime_error("Uniform '" + std::string(name) + "' not found");
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::set(const std::string_view name, const glm::vec2& value) const {
    GLint loc = glGetUniformLocation(programID_, name.data());
    if (loc == -1) throw std::runtime_error("Uniform '" + std::string(name) + "' not found");
    glUniform2fv(loc, 1, glm::value_ptr(value));
}

void Shader::set(const std::string_view name, const glm::vec3& value) const {
    GLint loc = glGetUniformLocation(programID_, name.data());
    if (loc == -1) throw std::runtime_error("Uniform '" + std::string(name) + "' not found");
    glUniform3fv(loc, 1, glm::value_ptr(value));
}

void Shader::set(const std::string_view name, const glm::vec4& value) const {
    GLint loc = glGetUniformLocation(programID_, name.data());
    if (loc == -1) throw std::runtime_error("Uniform '" + std::string(name) + "' not found");
    glUniform4fv(loc, 1, glm::value_ptr(value));
}

void Shader::set(const std::string_view name, float* value, int count) const {
    GLint loc = glGetUniformLocation(programID_, name.data());
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