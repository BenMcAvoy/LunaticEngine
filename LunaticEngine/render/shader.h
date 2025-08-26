#pragma once

#include "pch.h"

namespace Lunatic {
	constexpr const char* DEFAULT_VERTEX_SRC = R"(
#version 460 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texCoord;

uniform mat4 u_viewProjection;
uniform mat4 u_model;

out vec2 TexCoord;

void main() {
    gl_Position = u_viewProjection * u_model * vec4(position, 0.0, 1.0);
    TexCoord = texCoord;
}
)";

    constexpr const char* DEFAULT_FRAGMENT_SRC = R"(
#version 460 core

out vec4 FragColor;

uniform vec4 u_colour;

uniform bool u_useTexture = false;
uniform sampler2D u_texture;

in vec2 TexCoord;

void main() {
    //FragColor = u_colour;
    if (u_useTexture) {
        vec3 rgb = vec3(u_colour);
        float V = max(max(rgb.r, rgb.g), rgb.b);
        FragColor = texture(u_texture, TexCoord) * V;
    } else {
        FragColor = u_colour;
    }

	if (FragColor.a < 0.01) {
        discard;
    }
}
)";

// Shader class, helps to load and manage shaders
class Shader {
public:
    Shader(std::string_view vertexPath, std::string_view fragmentPath);
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
    void set(std::string_view name, GLuint value) const;
    void set(std::string_view name, const glm::mat4& value) const;
	void set(std::string_view name, const glm::vec2& value) const;
    void set(std::string_view name, const glm::vec3& value) const;
    void set(std::string_view name, const glm::vec4& value) const;
    void set(std::string_view name, float *value, int count) const;

private:
    GLuint programID_;

    // Helpers
    std::string loadShaderSource(std::string_view path) const;
    GLuint compileShader(GLenum type, const char* source) const;
    GLuint createProgram(GLuint vertex, GLuint fragment) const;
};
} // namespace Lunatic
