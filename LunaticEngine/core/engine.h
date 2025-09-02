#pragma once

#include "pch.h"

#include "model/base.h"

//#include "render/camera.h"
#include "render/renderer.h"
#include "render/shader.h"

#include "model/primitives/camera.h"

namespace Lunatic {
	enum class KeyAction {
		None,
		Pressed,
		Held,
		Released
	};

	enum class MouseButtonAction {
		None,
		Pressed,
		Held,
		Released
	};

	class Engine {
	public:
		static Engine& getInstance();

		Engine(const Engine&) = delete;
		Engine& operator=(const Engine&) = delete;
		Engine(Engine&&) = delete;
		Engine& operator=(Engine&&) = delete;

		Engine();
		~Engine();

		void init(GLFWwindow* externalWindow = nullptr, bool initLibs = true);

		void run();

		void clearAllLuaDataStores();

		KeyAction getKeyState(int key);
		MouseButtonAction getMouseButtonState(int button);
		double getMouseX() const;
		double getMouseY() const;
		glm::vec2 getMousePos() const;
		double getMouseDeltaX();
		double getMouseDeltaY();
		double getScrollDeltaX();
		double getScrollDeltaY();

		void setAutoResizeRenderer(bool enable) { autoResizeRenderer_ = enable; }

		void drawText(glm::vec2 position, const std::string& text, glm::vec4 colour);

		void hideCursor();
		void showCursor();

		// Resizes the window and updates the camera viewport
		void resize(int width, int height);

		std::shared_ptr<Instance> rootInstance = nullptr;

		void registerRenderable(Renderable* renderable);
		void unregisterRenderable(Renderable* renderable);

		void registerUpdateable(Updateable* updateable);
		void unregisterUpdateable(Updateable* updateable);

		void registerMainCamera(Camera* camera);

		void setWindowOffset(int x, int y) {
			windowOffsetX_ = x; windowOffsetY_ = y;
		}

		Renderer& getRenderer() { return renderer_; }
		std::vector<Renderable*>& getRenderables() { return renderables_; }
		std::vector<Updateable*>& getUpdateables() { return updateables_; }

	private:
		std::vector<Renderable*> renderables_;
		std::vector<Updateable*> updateables_;

		Renderer renderer_;

		// Debug UI state
		std::weak_ptr<Instance> selectedInstance_{};
		std::string inspectorNameBuffer_{};

		GLFWwindow* glfwWindow_ = nullptr;

		// For now we do key actions and mouse actions, we ignore joystick actions and other input types.
		std::array<KeyAction, GLFW_KEY_LAST + 1> keyActions_ = {};
		std::array<MouseButtonAction, GLFW_MOUSE_BUTTON_LAST + 1> mouseButtonActions_ = {};

		// Mouse data (e.g. position, scroll, delta, etc.)
		double mouseX_ = 0.0;
		double mouseY_ = 0.0;
		double mouseScrollX_ = 0.0;
		double mouseScrollY_ = 0.0;
		double mouseDeltaX_ = 0.0;
		double mouseDeltaY_ = 0.0;
		double mouseScrollDeltaX_ = 0.0;
		double mouseScrollDeltaY_ = 0.0;

		// Previous-frame mouse position (used to compute deltas correctly)
		double prevMouseX_ = 0.0;
		double prevMouseY_ = 0.0;
		bool mouseInWindow_ = false;

		// Window data
		int windowOffsetX_ = 0;
		int windowOffsetY_ = 0;
		int windowWidth_ = 800;
		int windowHeight_ = 600;
		const char* windowTitle_ = "Lunatic Engine";
		int windowPosX_ = 100;
		int windowPosY_ = 100;

		bool autoResizeRenderer_ = true;

		// GLFW callbacks (not all are useful, but all are provided for completeness)
		static void winErrorCallback(int error_code, const char* description);
		static void winWindowPosCallback(GLFWwindow* window, int xpos, int ypos);
		static void winWindowSizeCallback(GLFWwindow* window, int width, int height);
		static void winWindowCloseCallback(GLFWwindow* window);
		static void winWindowRefreshCallback(GLFWwindow* window);
		static void winWindowFocusCallback(GLFWwindow* window, int focused);
		static void winWindowIconifyCallback(GLFWwindow* window, int iconified);
		static void winWindowMaximizeCallback(GLFWwindow* window, int maximized);
		static void winFramebufferSizeCallback(GLFWwindow* window, int width, int height);
		static void winWindowContentScaleCallback(GLFWwindow* window, float xscale, float yscale);
		static void winKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
		static void winCharCallback(GLFWwindow* window, unsigned int codepoint);
		static void winCharModsCallback(GLFWwindow* window, unsigned int codepoint, int mods);
		static void winMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
		static void winCursorPosCallback(GLFWwindow* window, double xpos, double ypos);
		static void winCursorEnterCallback(GLFWwindow* window, int entered);
		static void winScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
		static void winJoystickCallback(int jid, int event);
		static void winDropCallback(GLFWwindow* window, int path_count, const char* paths[]);
	};
}
