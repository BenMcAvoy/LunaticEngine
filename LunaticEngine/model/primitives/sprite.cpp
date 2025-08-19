#include "pch.h"

#include "sprite.h"

#include "core/engine.h"

using namespace Lunatic;

const std::array<Vertex, 4> vertices = {
	Vertex(-0.5f, -0.5f, 0.0f, 0.0f), // Bottom-left
	Vertex(0.5f, -0.5f, 1.0f, 0.0f), // Bottom-right
	Vertex(0.5f, 0.5f, 1.0f, 1.0f),  // Top-right
	Vertex(-0.5f, 0.5f, 0.0f, 1.0f)  // Top-left
};

const GLuint indices[] = { 0, 1, 2, 2, 3, 0 };

Sprite::Sprite(const std::string_view name) : Instance(name), Renderable() {
	metaType = entt::resolve<Sprite>();

	if (!dataInitialized_) {
		std::println("Initializing static data for Sprite");

		// Initialize static data for the first time
		glGenVertexArrays(1, &vao_);
		glBindVertexArray(vao_);
		glGenBuffers(1, &vbo_);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
		glEnableVertexAttribArray(1);
		glGenBuffers(1, &ebo_);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

		dataInitialized_ = true;
	}
}

std::string Sprite::getClassName() const {
	return "Sprite";
}

void Sprite::draw() {
	glBindVertexArray(vao_);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}
