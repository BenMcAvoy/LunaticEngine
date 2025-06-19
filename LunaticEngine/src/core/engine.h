#pragma once

#include "pch.h"

#include "hierarchy/base.h"

#include "hierarchy/services/scripting.h"
#include "hierarchy/services/workspace.h"
#include "hierarchy/services/debug.h"

#include "render/shader.h"

namespace Lunatic {
	class Engine {
public:
	Engine(std::uint32_t width, std::uint32_t height, std::string_view title);
	~Engine();

	static Engine& GetInstance() {
		LUN_ASSERT(s_instance != nullptr, "Engine instance is null, did you forget to create it?")
		return *s_instance;
	}

	void run();
	void stop();

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
	// Access to services map for debug purposes
	const std::unordered_map<std::string, std::shared_ptr<Service>>& getServicesMap() const {
		return m_services;
	}

	// Input helper methods
	bool isKeyPressed(int key) const {
		if (key >= 0 && key < static_cast<int>(m_keyStates.size())) {
			return m_keyStates[key] == KeyState::Pressed;
		}
		return false;
	}

	bool isMouseButtonPressed(int button) const {
		if (button >= 0 && button < static_cast<int>(m_mousePressed.size())) {
			return m_mousePressed[button];
		}
		return false;
	}

	glm::vec2 getMousePosition() const { return m_mousePos; }
	glm::vec2 getWindowSize() const { return m_windowSize; }
	bool isWindowFocused() const { return m_windowFocused; }

private:
	// Entire engine is architected around services (e.g. workspace, lighting, environment, etc.)
	std::unordered_map<std::string, std::shared_ptr<Service>> m_services;

	GLFWwindow* m_window;
	bool m_running = false;

	glm::vec2 m_windowPos = { 0.0f, 0.0f };
	glm::vec2 m_windowSize = { 800.0f, 600.0f };
	glm::vec2 m_mousePos = { 0.0f, 0.0f };

	bool m_mouseInWindow = false;
	bool m_windowFocused = true;
	bool m_windowIconified = false;

	std::array<bool, 3> m_mousePressed = { false, false, false };

	enum class KeyState {
		Pressed,
		Released
	};
	std::array<KeyState, GLFW_KEY_LAST + 1> m_keyStates = {};

	// Window related callbacks
	static void CB_WindowPos(GLFWwindow* window, int xPos, int yPos);
	static void CB_WindowSize(GLFWwindow* window, int width, int height);
	static void CB_WindowClose(GLFWwindow* window);
	static void CB_WindowRefresh(GLFWwindow* window);
	static void CB_WindowFocus(GLFWwindow* window, int focused);
	static void CB_WindowIconify(GLFWwindow* window, int iconified);
	static void CB_WindowMaximize(GLFWwindow* window, int maximized);
	static void CB_WindowContentScale(GLFWwindow* window, float xScale, float yScale);
	static void CB_FramebufferSize(GLFWwindow* window, int width, int height);

	// Input related callbacks
	static void CB_Key(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void CB_Char(GLFWwindow* window, unsigned int codepoint);
	static void CB_CharMods(GLFWwindow* window, unsigned int codepoint, int mods);
	static void CB_MouseButton(GLFWwindow* window, int button, int action, int mods);
	static void CB_CursorPos(GLFWwindow* window, double xPos, double yPos);
	static void CB_CursorEnter(GLFWwindow* window, int entered);
	static void CB_Scroll(GLFWwindow* window, double xOffset, double yOffset);
	static void CB_JoyStick(int jid, int event);

	// Miscellaneous callbacks
	static void CB_Monitor(GLFWmonitor* monitor, int event);
	static void CB_Drop(GLFWwindow* window, int count, const char** paths);
	static void CB_Error(int error, const char* description);

	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;
	Engine(Engine&&) = delete;
	Engine& operator=(Engine&&) = delete;
	static Engine* s_instance;
};
} // namespace Lunatic
