#include "pch.h"

#include "cube.h"

using namespace Lunatic;

Cube::Cube(std::string_view name) : Instance(name, "Cube") {
	static bool hasUploaded = false;

	if (!hasUploaded) {
		if (!sm_buffers.has_value()) {
			sm_buffers.emplace();
		}

		sm_buffers->uploadData(
			std::span<const float>(sm_vertices.data(), sm_vertices.size()),
			std::span<const std::uint32_t>(sm_indices.data(), sm_indices.size())
		);
		// Set up vertex attributes for 3D cube (position + normal + texture coords)
		sm_buffers->bind();
		sm_buffers->setAttribute(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0); // Position
		sm_buffers->setAttribute(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (const void*)(3 * sizeof(float))); // Normal
		sm_buffers->setAttribute(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (const void*)(6 * sizeof(float))); // Texture coords

		hasUploaded = true;
	}
}

void Cube::render() {
	// The renderer service handles shader setup and model matrix.
	// This method only binds the cube's geometry and draws it.
	// TODO: Change this
	
	sm_buffers->bind();
	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(sm_indices.size()), GL_UNSIGNED_INT, nullptr);
}