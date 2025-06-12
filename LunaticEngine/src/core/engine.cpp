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
	glfwSwapInterval(1);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) throw std::runtime_error("Failed to initialize GLAD");
	glfwSetWindowUserPointer(m_window, this);
	glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, int width, int height) {
		auto engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
		engine->m_windowSize = { static_cast<float>(width), static_cast<float>(height) };
		glViewport(0, 0, width, height);

		static auto renderer = ServiceLocator::Get<Services::Renderer>("Renderer");
		renderer->resize(width, height);
		});
	glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double xPos, double yPos) {
		auto engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
		engine->m_mousePos = { static_cast<float>(xPos), static_cast<float>(yPos) };
		});
	glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int button, int action, int mods) {
		auto engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
		engine->m_mousePressed[button] = (action == GLFW_PRESS);
		});
	glfwSetScrollCallback(m_window, [](GLFWwindow* window, double xOffset, double yOffset) {
		auto engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
		engine->m_mousePos.y += static_cast<float>(yOffset);
		});
	glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
		// TODO: Handle key events
		auto engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
		if (action == GLFW_PRESS) {
			if (key == GLFW_KEY_ESCAPE) {
				engine->m_running = false;
				glfwSetWindowShouldClose(window, true);
			}
		}
		});

	// Initialize ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
#ifdef _WIN32
	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Arial.ttf", 16.0f);
	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Arial.ttf", 24.0f);
#endif

	// Enable docking
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();

	// make it super rounded and pretty
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

		glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Update services
		debug->update(0.01666f);

		// Render services
		debug->render();

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