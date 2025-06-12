#include "pch.h"

#include "renderer.h"
#include "workspace.h"

#ifdef _DEBUG
#include <spdlog/spdlog.h>
#include <glad/glad.h>

void GLAPIENTRY openglDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    spdlog::warn("[OpenGL][{}][{}][{}] {}", id, type, severity, message);
}
#endif

using namespace Lunatic::Services;

// Quad vertices
std::array<float, 12> quadVertices = {
	-1.0f, -1.0f, 0.0f, // Bottom-left
	 1.0f, -1.0f, 0.0f, // Bottom-right
	-1.0f,  1.0f, 0.0f, // Top-left
	 1.0f,  1.0f, 0.0f  // Top-right
};

// Quad indices
std::array<unsigned int, 6> quadIndices = {
	0, 1, 2, // First triangle
	1, 3, 2  // Second triangle
};

Renderer::Renderer() : Service("Renderer") {
#ifdef _DEBUG
    GLint flags; glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(openglDebugCallback, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        spdlog::debug("OpenGL debug context enabled");
    } else {
        spdlog::warn("OpenGL debug context not available");
    }
#endif

	m_camera.setZoom(10.0f);
	m_buffers.uploadData(std::span<float>(quadVertices.data(), quadVertices.size()),
		std::span<unsigned int>(quadIndices.data(), quadIndices.size()));
	m_buffers.bind(); // Ensure VAO is bound before setting attribute
	m_buffers.setAttribute(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
}

void Renderer::update(float deltatime) {
	// Nothing to update for now. (we render only at the moment)
}

void Renderer::render() {
	m_shader.use();

	m_shader.set("u_viewProjection", m_camera.getViewProjection());
	m_shader.set("u_colour", glm::vec3(1.0f, 0.0f, 0.0f));
	m_shader.set("u_model", glm::mat4(1.0f));

	/*m_buffers.bind();

	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(quadIndices.size()), GL_UNSIGNED_INT, nullptr);*/

	/// ^^^ yap yap yap ^^^

	static auto workspace = ServiceLocator::Get<Services::Workspace>("Workspace");
	auto instances = workspace->getInstances();

	m_buffers.bind(); // Bind the VAO/VBO/EBO before rendering
	std::function<void(std::shared_ptr<Instance>, glm::vec3)> renderInstance;
	renderInstance = [&](std::shared_ptr<Instance> instance, glm::vec3 parentPosition) {
		glm::mat4 model = glm::mat4(1.0f);
		glm::vec3 position = glm::vec3(instance->position.x, instance->position.y, 0.0f) + parentPosition;
		model = glm::translate(model, position);

		// Set values into shader (u_viewProjection is already set)
		m_shader.set("u_model", model);

		// Render using glDrawElements
		glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(quadIndices.size()), GL_UNSIGNED_INT, nullptr);

		// Render children recursively
		for (const auto& child : instance->children) {
			renderInstance(child, position);
		}
		};

	for (const auto& instance : instances)
		renderInstance(instance, glm::vec3(0.0f));
}

void Renderer::resize(int width, int height) {
	m_camera.resize(width, height);
}
