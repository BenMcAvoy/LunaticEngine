#pragma once

#include "pch.h"

namespace Lunatic {
	struct Vertex {
		glm::vec3 position;
		glm::vec2 texCoord;
	};

	/// <summary>
	/// Represents a collection of buffers used for rendering.
	/// </summary>
	class Buffers {
	public:
		Buffers();
		~Buffers();

		void uploadData(std::span<const float> vertexData, std::span<const std::uint32_t> indexData, GLenum usage = GL_STATIC_DRAW) const;
		void setAttribute(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer) const;
		void bind() const;

	private:
		unsigned int m_vao = 0; // Vertex Array Object
		unsigned int m_vbo = 0; // Vertex Buffer Object
		unsigned int m_ebo = 0; // Element Buffer Object

		Buffers(const Buffers&) = delete;
		Buffers& operator=(const Buffers&) = delete;
	};
} // namespace Lunatic
