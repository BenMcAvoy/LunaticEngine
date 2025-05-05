#include "pch.h"

#include "lunatic/buffers.h"

Lunatic::Buffers::Buffers() {
	glGenVertexArrays(1, &m_vao);
	glGenBuffers(1, &m_vbo);
	glGenBuffers(1, &m_ebo);

	spdlog::info("Buffers::Buffers - Created VAO: {}, VBO: {}, EBO: {}", m_vao, m_vbo, m_ebo);
}

Lunatic::Buffers::~Buffers() {
	glDeleteVertexArrays(1, &m_vao);
	glDeleteBuffers(1, &m_vbo);
	glDeleteBuffers(1, &m_ebo);
}

void Lunatic::Buffers::uploadData(std::span<const float> vertexData, std::span<const std::uint32_t> indexData, GLenum usage) const {
	glBindVertexArray(m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, vertexData.size_bytes(), vertexData.data(), usage);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexData.size_bytes(), indexData.data(), usage);

	spdlog::info("Buffers::uploadData - Uploaded {} vertices and {} indices", vertexData.size() / 4, indexData.size());
}

void Lunatic::Buffers::setAttribute(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer) const {
	glBindVertexArray(m_vao);
	glVertexAttribPointer(index, size, type, normalized, stride, pointer);
	glEnableVertexAttribArray(index);

	spdlog::info("Buffers::setAttribute - Set attribute {}: size={}, type={}, normalized={}, stride={}, pointer={}", index, size, type, normalized, stride, pointer);
}

void Lunatic::Buffers::bind() const {
	glBindVertexArray(m_vao);
}
