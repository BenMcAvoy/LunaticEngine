#pragma once

#include "pch.h"

#include "lunatic/scene.h"
#include "lunatic/shader.h"

namespace Lunatic {
class Engine {
public:
	Engine(std::uint32_t width, std::uint32_t height, std::string_view title);
	~Engine();

	static Engine& GetInstance() {
		LUN_ASSERT(s_instance != nullptr, "Engine instance is null, did you forget to create it?");
		return *s_instance;
	}

	void run();
	void stop() { m_running = false; if (m_window) glfwSetWindowShouldClose(m_window, true); }

	std::shared_ptr<Scene> getScene() { return m_scene; }

private:
	void drawObjectRecursive(const std::shared_ptr<Object>& obj, const glm::mat4& parentModel);
	void debugInspector();

	GLFWwindow* m_window;
	bool m_running = false;

	glm::vec2 m_windowSize; // changed on callback
	glm::vec2 m_mousePos; // changed on callback
	std::array<bool, 3> m_mousePressed = { false, false, false };

	std::shared_ptr<Scene> m_scene = nullptr;

	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;
	Engine(Engine&&) = delete;
	Engine& operator=(Engine&&) = delete;
	static Engine* s_instance;
};
} // namespace Lunatic
