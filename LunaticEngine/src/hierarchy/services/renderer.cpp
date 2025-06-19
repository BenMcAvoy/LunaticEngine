#include "pch.h"

#include "renderer.h"
#include "workspace.h"

#include "core/engine.h"

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

	// Enable depth testing for 3D rendering
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	m_camera.setFOV(45.0f);
	m_buffers.uploadData(std::span<float>(quadVertices.data(), quadVertices.size()),
		std::span<unsigned int>(quadIndices.data(), quadIndices.size()));
	m_buffers.bind(); // Ensure VAO is bound before setting attribute
	m_buffers.setAttribute(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
}

void Renderer::update(float deltatime) {
	updateCameraControls(deltatime);
}

void Renderer::render() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	const auto& bgColor = m_camera.getBackgroundColor();
	glClearColor(bgColor.r, bgColor.g, bgColor.b, 1.0f);

	m_shader.use();
	m_shader.set("u_viewProjection", m_camera.getViewProjection());
	static auto workspace = ServiceLocator::Get<Services::Workspace>("Workspace");
	auto instances = workspace->getInstances();

	std::function<void(std::shared_ptr<Instance>, glm::mat4)> renderInstance;
	renderInstance = [&](std::shared_ptr<Instance> instance, glm::mat4 parentTransform) {
		// Start with the parent's transformation matrix
		glm::mat4 model = parentTransform;
		
		// Apply this instance's local transformations
		model = glm::translate(model, instance->position);
		model = glm::rotate(model, glm::radians(instance->rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(instance->rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(instance->rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		// Set model matrix and color for this instance
		m_shader.set("u_model", model);
		m_shader.set("u_color", glm::vec3(1.0f, 0.5f, 0.2f)); // Orange color for cubes

		// Call the instance's render method (which will bind its own geometry and draw)
		instance->render();

		// Render children recursively, passing the current transformation matrix
		for (const auto& child : instance->children) {
			renderInstance(child, model);
		}
		};

	for (const auto& instance : instances)
		renderInstance(instance, glm::mat4(1.0f));
}

void Renderer::resize(int width, int height) {
	m_camera.resize(width, height);
}

void Renderer::updateCameraControls(float deltaTime) {
	auto& engine = Engine::GetInstance();
	
	// Toggle camera controls with F1
	static bool f1Pressed = false;
	if (engine.isKeyPressed(GLFW_KEY_F1)) {
		if (!f1Pressed) {
			m_cameraControlEnabled = !m_cameraControlEnabled;
			m_firstMouse = true; // Reset mouse on toggle
			
			// Set cursor mode based on camera control state
			GLFWwindow* window = glfwGetCurrentContext();
			if (m_cameraControlEnabled) {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			} else {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			}
		}
		f1Pressed = true;
	} else {
		f1Pressed = false;
	}
	
	if (!m_cameraControlEnabled || !engine.isWindowFocused()) {
		return;
	}
	
	// WASD movement
	glm::vec3 movement(0.0f);
	float speed = m_cameraSpeed * deltaTime;
	
	if (engine.isKeyPressed(GLFW_KEY_W)) {
		movement += m_camera.getForward() * speed;
	}
	if (engine.isKeyPressed(GLFW_KEY_S)) {
		movement -= m_camera.getForward() * speed;
	}
	if (engine.isKeyPressed(GLFW_KEY_A)) {
		movement -= m_camera.getRight() * speed;
	}
	if (engine.isKeyPressed(GLFW_KEY_D)) {
		movement += m_camera.getRight() * speed;
	}
	if (engine.isKeyPressed(GLFW_KEY_SPACE)) {
		movement += m_camera.getUp() * speed;
	}
	if (engine.isKeyPressed(GLFW_KEY_LEFT_SHIFT)) {
		movement -= m_camera.getUp() * speed;
	}
	
	// Apply movement
	if (glm::length(movement) > 0.0f) {
		m_camera.translate(movement);
	}
	
	// Mouse look
	glm::vec2 mousePos = engine.getMousePosition();
	
	if (m_firstMouse) {
		m_lastMousePos = mousePos;
		m_firstMouse = false;
	}
	
	glm::vec2 mouseDelta = mousePos - m_lastMousePos;
	m_lastMousePos = mousePos;
	
	// Apply mouse sensitivity
	mouseDelta *= m_mouseSensitivity;
	
	m_yaw += mouseDelta.x;
	m_pitch -= mouseDelta.y; // Inverted Y-axis
	
	// Constrain pitch
	m_pitch = glm::clamp(m_pitch, -89.0f, 89.0f);
	
	// Update camera rotation
	m_camera.setRotation(m_pitch, m_yaw, 0.0f);
}
