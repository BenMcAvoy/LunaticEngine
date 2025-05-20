#include "pch.h"

#include "engine.h"

using namespace Lunatic;

#ifdef _DEBUG
class ImGuiConsole {
public:
	ImGuiConsole() : auto_scroll_(true), scroll_to_bottom_(false) {
		Clear();
	}

	// Clear all log entries
	void Clear() {
		logs_.clear();
		filter_.Clear();
	}

	// Add a colored log entry
	void AddLog(const ImVec4& col, const char* fmt, ...) IM_FMTARGS(3) {
		char buf[1024];
		va_list args;
		va_start(args, fmt);
		std::vsnprintf(buf, sizeof(buf), fmt, args);
		va_end(args);

		logs_.push_back({ std::string(buf), col });
		if (auto_scroll_) {
			scroll_to_bottom_ = true;
		}
	}

	// Render console window
	void Draw(const char* title, bool* p_open = nullptr) {
		if (!ImGui::Begin(title, p_open)) {
			ImGui::End();
			return;
		}

		// Controls
		if (ImGui::Button("Options")) ImGui::OpenPopup("ConsoleOptions");
		ImGui::SameLine();
		if (ImGui::Button("Clear")) Clear();
		ImGui::SameLine();
		if (ImGui::Button("Copy")) ImGui::LogToClipboard();
		ImGui::SameLine();
		filter_.Draw("Filter (inc,-exc)");

		if (ImGui::BeginPopup("ConsoleOptions")) {
			ImGui::Checkbox("Auto-scroll", &auto_scroll_);
			ImGui::EndPopup();
		}

		ImGui::Separator();

		// Main scrolling region
		ImGui::BeginChild(
			"ScrollingRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()),
			false, ImGuiWindowFlags_HorizontalScrollbar);

		for (const auto& entry : logs_) {
			if (!filter_.IsActive() || filter_.PassFilter(entry.message.c_str())) {
				ImGui::PushStyleColor(ImGuiCol_Text, entry.color);
				ImGui::TextUnformatted(entry.message.c_str());
				ImGui::PopStyleColor();
			}
		}

		if (scroll_to_bottom_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
			ImGui::SetScrollHereY(1.0f);
		}
		scroll_to_bottom_ = false;

		ImGui::EndChild();
		ImGui::Separator();

		// No input area

		ImGui::End();
	}

private:
	std::vector<LogContainer> logs_;  // Stored log entries
	ImGuiTextFilter filter_;          // Text filter
	bool auto_scroll_;                // Auto-scroll to new entries
	bool scroll_to_bottom_;           // Flag to scroll on next frame
};

static ImGuiConsole console;

class CustomSink : public spdlog::sinks::base_sink<std::mutex> {
public:
	CustomSink() = default;

protected:
	void sink_it_(const spdlog::details::log_msg& msg) override {
		spdlog::memory_buf_t formatted;
        formatter_->format(msg, formatted);
		std::string formattedMessage = fmt::to_string(formatted);
		auto color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // default white
		switch (msg.level) {
			// gray (trace)
		case spdlog::level::trace:
			color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
			break;
			// green (info)
		case spdlog::level::info:
			color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
			break;
			// yellow (warn)
		case spdlog::level::warn:
			color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
			break;
			// red (err/critical)
		case spdlog::level::err:
		case spdlog::level::critical:
			color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
			break;
		}

		console.AddLog(color, "%s", formattedMessage.c_str());
	};

	void flush_() override {
		// Flush is not needed for this sink
	}
};
#endif

Engine* Lunatic::Engine::s_instance = nullptr;
Engine::Engine(std::uint32_t width, std::uint32_t height, std::string_view title)
	: m_windowSize(width, height), m_mousePos(0.0f, 0.0f) {
#ifdef _DEBUG
	// apply custom sink and console sink

	auto customSink = std::make_shared<CustomSink>();
	spdlog::sinks_init_list sinks = { customSink };
	spdlog::set_default_logger(std::make_shared<spdlog::logger>("Lunatic", sinks));
	spdlog::set_level(spdlog::level::trace);
#endif

	LUN_ASSERT(s_instance == nullptr, "Engine instance already exists, did you forget to destroy it?")
	s_instance = this;

	if (!glfwInit()) throw std::runtime_error("Failed to initialize GLFW");
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	m_window = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
	if (!m_window) throw std::runtime_error("Failed to create GLFW window");
	glfwMakeContextCurrent(m_window);
	glfwSwapInterval(1);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) throw std::runtime_error("Failed to initialize GLAD");
	glfwSetWindowUserPointer(m_window, this);
	glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int width, int height) {
		auto engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
		engine->m_windowSize = { static_cast<float>(width), static_cast<float>(height) };
		glViewport(0, 0, width, height);
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
	while (!glfwWindowShouldClose(m_window)) {
		glfwPollEvents();

		glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Lunatic services");
		{
			if (ImGui::BeginTable("ServicesTable", 5, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoBordersInBody)) {
				static std::unordered_map<std::string, bool> autoUpdateMap;
				static std::unordered_map<std::string, bool> autoRenderMap;

				for (const auto& [name, service] : m_services) {
					ImGui::PushID(name.data());

					autoUpdateMap.try_emplace(name, false);
					autoRenderMap.try_emplace(name, false);

					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex(0);
					ImGui::Text("%s", name.data());

					ImGui::TableSetColumnIndex(1);
					if (ImGui::Button("Update")) {
						service->update(0.01666f);
					}

					ImGui::TableSetColumnIndex(2);
					ImGui::Checkbox("Auto Update", &autoUpdateMap[name]);

					ImGui::TableSetColumnIndex(3);
					if (ImGui::Button("Render")) {
						auto renderableService = std::dynamic_pointer_cast<IRenderable>(service);
						if (renderableService) {
							renderableService->render();
						}
						else {
							spdlog::warn("Service {} is not a RenderableService", name);
						}
					}

					ImGui::TableSetColumnIndex(4);
					ImGui::Checkbox("Auto Render", &autoRenderMap[name]);

					ImGui::PopID();

					if (autoUpdateMap[name]) {
						service->update(0.01666f);
					}

					if (autoRenderMap[name]) {
						auto renderableService = std::dynamic_pointer_cast<IRenderable>(service);
						if (renderableService) {
							renderableService->render();
						}
					}
				}
				ImGui::EndTable();
			}
		}
		ImGui::End();

		auto lm = ServiceLocator::Get<Services::Scripting>("Scripting");
		lm->drawImGuiWindow();

#ifdef _DEBUG
		console.Draw("Console");
#endif

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(m_window);
	}

	m_running = false;
}

Engine::~Engine() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwDestroyWindow(m_window);
	glfwTerminate();

	s_instance = nullptr;
}