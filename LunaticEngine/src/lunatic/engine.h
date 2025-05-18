#pragma once

#include "pch.h"

#include "datamodel.h"
#include "shader.h"

namespace Lunatic {
#ifdef _DEBUG
	struct LogContainer {
		std::string message;
		ImVec4 color;
	};
#endif

class Engine {
public:
	Engine(std::uint32_t width, std::uint32_t height, std::string_view title);
	~Engine();

	static Engine& GetInstance() {
		LUN_ASSERT(s_instance != nullptr, "Engine instance is null, did you forget to create it?")
		return *s_instance;
	}

	void run();
	void stop() { m_running = false; if (m_window) glfwSetWindowShouldClose(m_window, true); }

	template <typename T, typename... Args>
	void registerService(std::string_view name, Args&&... args) {
		LUN_ASSERT(m_services.find(name.data()) == m_services.end(), "Service already registered")
		m_services[name.data()] = std::make_shared<T>(std::forward<Args>(args)...);
		ServiceLocator::Register(m_services[name.data()]);
	}

	std::shared_ptr<Service> getService(std::string_view name) {
		auto it = m_services.find(name.data());
		LUN_ASSERT(it != m_services.end(), "Service not found")
		return it->second;
	}

	template <typename T>
	std::shared_ptr<T> getService(std::string_view name) {
		auto it = m_services.find(name.data());
		LUN_ASSERT(it != m_services.end(), "Service not found")
		return std::dynamic_pointer_cast<T>(it->second);
	}

private:
	// Entire engine is architected around services (e.g. workspace, lighting, environment, etc.)
	std::unordered_map<std::string, std::shared_ptr<Service>> m_services;

	GLFWwindow* m_window;
	bool m_running = false;

	glm::vec2 m_windowSize; // changed on callback
	glm::vec2 m_mousePos; // changed on callback
	std::array<bool, 3> m_mousePressed = { false, false, false };

	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;
	Engine(Engine&&) = delete;
	Engine& operator=(Engine&&) = delete;
	static Engine* s_instance;
};
} // namespace Lunatic
