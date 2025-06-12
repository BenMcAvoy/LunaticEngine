#pragma once

#include "pch.h"

namespace Lunatic {

	constexpr const char* DEFAULT_VERTEX_SRC = R"(
#version 460 core

layout(location = 0) in vec3 aPos;

uniform mat4 u_model;
uniform mat4 u_viewProjection;

void main() {
  gl_Position = u_viewProjection * u_model * vec4(aPos, 1.0);
}
)";

constexpr const char* DEFAULT_FRAGMENT_SRC = R"(
  #version 460 core

  out vec4 FragColor;

  uniform vec3 u_colour;

  void main() {
    FragColor = vec4(u_colour, 1.0);
  }
)";

// Shader class, helps to load and manage shaders
class Shader {
public:
  Shader(const std::string &vertexPath, const std::string &fragmentPath);
  Shader(); // Uses default shaders

  ~Shader();

  // Apply the shader to the current OpenGL context
  void use() const;

  // Utility uniform functions for setting values
  // without using many different functions
  void set(std::string_view name, float value) const;
  void set(std::string_view name, int value) const;
  void set(std::string_view name, bool value) const;
  void set(std::string_view name, GLfloat *value) const;
  void set(std::string_view name, const glm::mat4& value) const;
  void set(std::string_view name, const glm::vec3& value) const;
  void set(std::string_view name, float *value, int count) const;

private:
    unsigned int m_id;

    // Helpers
    std::string loadShaderSource(const std::string& path) const;
    unsigned int compileShader(unsigned int type, const char* source) const;
    unsigned int createProgram(unsigned int vertex, unsigned int fragment) const;
};
} // namespace Lunatic
