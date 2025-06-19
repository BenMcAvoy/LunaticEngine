#pragma once

#include "pch.h"

namespace Lunatic {

	constexpr const char* DEFAULT_VERTEX_SRC = R"(
#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

out vec3 fragNormal;
//out vec2 fragTexCoord;

uniform mat4 u_viewProjection;
uniform mat4 u_model;

void main() {
	gl_Position = u_viewProjection * u_model * vec4(position, 1.0);
	
	// Transform normal to world space
	fragNormal = mat3(u_model) * normal;
	
	//fragTexCoord = texCoord;
}
)";

constexpr const char* DEFAULT_FRAGMENT_SRC = R"(
#version 460 core

in vec3 fragNormal;
//in vec2 fragTexCoord;

out vec3 FragColor;

uniform vec3 u_color;

void main() {
	vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3)); // Fake light direction
	float ambient = 0.3;
	float lightIntensity = 1.0;
	float directional = max(dot(fragNormal, lightDir), 0.0) * 0.7;
	float lighting = ambient + directional;
	FragColor = u_color * lighting;
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
