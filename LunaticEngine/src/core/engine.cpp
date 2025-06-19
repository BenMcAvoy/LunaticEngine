#include "pch.h"

#include "engine.h"

#include "hierarchy/services/renderer.h"
#include "hierarchy/services/debug.h"

using namespace Lunatic;

Engine* Lunatic::Engine::s_instance = nullptr;
Engine::Engine(std::uint32_t width, std::uint32_t height, std::string_view title)
	: m_windowSize(width, height), m_mousePos(0.0f, 0.0f) {
	
	LUN_ASSERT(s_instance == nullptr, "Engine instance already exists, did you forget to destroy it?")
	s_instance = this;

	if (!glfwInit()) throw std::runtime_error("Failed to initialize GLFW");
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
#ifdef _DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

	m_window = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
	if (!m_window) throw std::runtime_error("Failed to create GLFW window");
	glfwMakeContextCurrent(m_window);
	glfwSwapInterval(1); // TODO: option for this

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		throw std::runtime_error("Failed to initialize GLAD");

	glfwSetWindowUserPointer(m_window, this);

	// Set GLFW callbacks
	glfwSetWindowPosCallback(m_window, CB_WindowPos);
	glfwSetWindowSizeCallback(m_window, CB_WindowSize);
	glfwSetWindowCloseCallback(m_window, CB_WindowClose);
	glfwSetWindowRefreshCallback(m_window, CB_WindowRefresh);
	glfwSetWindowFocusCallback(m_window, CB_WindowFocus);
	glfwSetWindowIconifyCallback(m_window, CB_WindowIconify);
	glfwSetWindowMaximizeCallback(m_window, CB_WindowMaximize);
	glfwSetWindowContentScaleCallback(m_window, CB_WindowContentScale);
	glfwSetFramebufferSizeCallback(m_window, CB_FramebufferSize);
	glfwSetKeyCallback(m_window, CB_Key);
	glfwSetCursorPosCallback(m_window, CB_CursorPos);
	glfwSetMouseButtonCallback(m_window, CB_MouseButton);
	glfwSetCursorEnterCallback(m_window, CB_CursorEnter);
	glfwSetScrollCallback(m_window, CB_Scroll);
	glfwSetCharCallback(m_window, CB_Char);
	glfwSetCharModsCallback(m_window, CB_CharMods);
	glfwSetJoystickCallback(CB_JoyStick);
	glfwSetMonitorCallback(CB_Monitor);
	glfwSetDropCallback(m_window, CB_Drop);
	glfwSetErrorCallback(CB_Error);

	// Initialize key states
	m_keyStates.fill(KeyState::Released);

	// Initialize ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
#ifdef _WIN32
	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Arial.ttf", 16.0f);
	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Arial.ttf", 24.0f);
#endif
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();

	style.WindowRounding = 5.0f;
	style.FrameRounding = 5.0f;
	style.GrabRounding = 5.0f;
	style.PopupRounding = 5.0f;
	style.ScrollbarRounding = 5.0f;
	style.TabRounding = 5.0f;
	style.WindowTitleAlign = ImVec2(0.5f, 0.5f);

	ImGui_ImplGlfw_InitForOpenGL(m_window, true);
	ImGui_ImplOpenGL3_Init("#version 460");
}

void Engine::run() {
	m_running = true;

	auto renderer = ServiceLocator::Get<Services::Renderer>("Renderer");
	auto workspace = ServiceLocator::Get<Services::Workspace>("Workspace");
	auto scripting = ServiceLocator::Get<Services::Scripting>("Scripting");
	auto debug = ServiceLocator::Get<Services::Debug>("Debug");

	workspace->initialize();
	renderer->resize(static_cast<int>(m_windowSize.x), static_cast<int>(m_windowSize.y));

	while (!glfwWindowShouldClose(m_window)) {
		glfwPollEvents();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Call update and render on every single service
		for (const auto& [name, service] : m_services) {
			constexpr float fakeDt = 1.0f / 60.0f;
			service->update(fakeDt);
			service->render();
		}

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(m_window);
	}

	m_running = false;
}

void Engine::stop() {
	m_running = false;
	if (m_window) {
		glfwSetWindowShouldClose(m_window, true);
	}
}

Engine::~Engine() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwDestroyWindow(m_window);
	glfwTerminate();

	s_instance = nullptr;
}

void Engine::CB_WindowPos(GLFWwindow* window, int xPos, int yPos) {
	auto engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
	engine->m_windowPos = { static_cast<float>(xPos), static_cast<float>(yPos) };
}
void Engine::CB_WindowSize(GLFWwindow* window, int width, int height) {
	auto engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
	engine->m_windowSize = { static_cast<float>(width), static_cast<float>(height) };
}
void Engine::CB_WindowClose(GLFWwindow* window) {
	// Not currently needed
}
void Engine::CB_WindowRefresh(GLFWwindow* window) {
	// Not currently needed (games are immediate mode)
}
void Engine::CB_WindowFocus(GLFWwindow* window, int focused) {
	auto engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
	engine->m_windowFocused = (focused == GLFW_TRUE);
}
void Engine::CB_WindowIconify(GLFWwindow* window, int iconified) {
	auto engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
	engine->m_windowIconified = (iconified == GLFW_TRUE);
}
void Engine::CB_WindowMaximize(GLFWwindow* window, int maximized) {
	auto engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
	engine->m_windowFocused = (maximized == GLFW_TRUE);
}
void Engine::CB_WindowContentScale(GLFWwindow* window, float xScale, float yScale) {
	// Not currently needed, but could be used for high DPI displays
}
void Engine::CB_FramebufferSize(GLFWwindow* window, int width, int height) {
	auto engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
	engine->m_windowSize = { static_cast<float>(width), static_cast<float>(height) };
	glViewport(0, 0, width, height);

	static auto renderer = ServiceLocator::Get<Services::Renderer>("Renderer");
	renderer->resize(width, height);
}
void Engine::CB_Key(GLFWwindow* window, int key, int scancode, int action, int mods) {
	auto engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_ESCAPE) {
			engine->m_running = false;
			glfwSetWindowShouldClose(window, true);
		}
	}

	// Store in key state array
	if (key >= 0 && key < static_cast<int>(engine->m_keyStates.size())) {
		if (action == GLFW_PRESS) {
			engine->m_keyStates[key] = KeyState::Pressed;
		} else if (action == GLFW_RELEASE) {
			engine->m_keyStates[key] = KeyState::Released;
		}
	}
}
void Engine::CB_Char(GLFWwindow* window, unsigned int codepoint) {
	// Not currently needed, but could be used for text input
}
void Engine::CB_CharMods(GLFWwindow* window, unsigned int codepoint, int mods) {
	// Not currently needed, but could be used for text input with modifiers
}
void Engine::CB_MouseButton(GLFWwindow* window, int button, int action, int mods) {
	auto engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
	engine->m_mousePressed[button] = (action == GLFW_PRESS);
}
void Engine::CB_CursorPos(GLFWwindow* window, double xPos, double yPos) {
	auto engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
	engine->m_mousePos = { static_cast<float>(xPos), static_cast<float>(yPos) };
}
void Engine::CB_CursorEnter(GLFWwindow* window, int entered) {
	auto engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
	engine->m_mouseInWindow = (entered == GLFW_TRUE);
	if (!engine->m_mouseInWindow) {
		engine->m_mousePos = { 0.0f, 0.0f }; // Reset mouse position when leaving window
	}
}
void Engine::CB_Scroll(GLFWwindow* window, double xOffset, double yOffset) {
	auto engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
	engine->m_mousePos.y += static_cast<float>(yOffset);
}
void Engine::CB_JoyStick(int jid, int event) {
	// TODO: Handle joystick events if needed
}
void Engine::CB_Monitor(GLFWmonitor* monitor, int event) {
	// Not currently needed, but could be used for monitor events like connection/disconnection
}
void Engine::CB_Drop(GLFWwindow* window, int count, const char** paths) {
	auto engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
	for (int i = 0; i < count; ++i) {
		spdlog::debug("Dropped file: {}", paths[i]);
		// Not currently needed, but could be used for file drag-and-drop functionality
	}
}
void Engine::CB_Error(int error, const char* description) {
	spdlog::error("GLFW Error {}: {}", error, description);
}
