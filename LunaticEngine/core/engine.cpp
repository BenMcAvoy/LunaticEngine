#include "pch.h"

#include "engine.h"
#include "render/renderer.h"

#include "model/reflection.h"

using namespace Lunatic;

Engine& Engine::getInstance() {
	static Engine instance;
	return instance;
}

template <size_t MAX_FRAMES>
struct StatBuffer {
    std::array<float, MAX_FRAMES> values{};
    int index = 0;

    void add(float v) {
        values[index] = v;
        index = (index + 1) % MAX_FRAMES;
    }

    const float* data() const { return values.data(); }
    int offset() const { return index; }
    int size() const { return MAX_FRAMES; }
};

void renderStats(float dt, int renderableCount) {
    static constexpr int MAX_FRAMES = 240; // ~4 seconds at 60fps
    static StatBuffer<MAX_FRAMES> frameTimes;
    static StatBuffer<MAX_FRAMES> fpsValues;
    static StatBuffer<MAX_FRAMES> renderableCounts;

    // Frame timing
    float frameTimeMs = dt * 1000.0f;
    float fps = (dt > 0.0f) ? 1.0f / dt : 0.0f;

    // Record values
    frameTimes.add(frameTimeMs);
    fpsValues.add(fps);
    renderableCounts.add((float)renderableCount);

    // Window
    ImGui::Begin("Performance Stats");

    ImGui::PlotLines(
        "Frame Time", frameTimes.data(), frameTimes.size(),
        frameTimes.offset(), nullptr,
        0.0f, 50.0f, ImVec2(0, 80)
    );

    // FPS
    ImGui::PlotLines(
        "Frames Per Second", fpsValues.data(), fpsValues.size(),
        fpsValues.offset(), nullptr,
        0.0f, 120.0f, ImVec2(0, 80)
    );

    // Entity Count
    ImGui::PlotLines(
        "Renderable Count", renderableCounts.data(), renderableCounts.size(),
        renderableCounts.offset(), nullptr,
        0.0f, 5000.0f, ImVec2(0, 80)
    );

    // Some live numbers
    ImGui::Separator();
    ImGui::Text("Current Frame Time: %.2f ms", frameTimeMs);
    ImGui::Text("Current FPS: %.1f", fps);
	ImGui::Text("Instance Count: %d", renderableCount);

	ImGui::Separator();

	// Manipulation of hierarchy stuff
	if (ImGui::Button("New Instance")) {
		auto newInstance = std::make_shared<Instance>("New Instance");
		newInstance->setParent(Engine::getInstance().rootInstance);
	}

    ImGui::End();
}

Engine::Engine() {
	if (!glfwInit()) {
		std::println("Failed to initialize GLFW");
		return;
	}

	glfwSetErrorCallback(winErrorCallback);
	glfwWindow_ = glfwCreateWindow(800, 600, "Lunatic Engine", nullptr, nullptr);
	if (!glfwWindow_) {
		std::println("Failed to create GLFW window");
		glfwTerminate();
		return;
	}

	glfwMakeContextCurrent(glfwWindow_);
	glfwSwapInterval(1);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::println("Failed to initialize GLAD");
		glfwDestroyWindow(glfwWindow_);
		glfwTerminate();
		return;
	}

	glfwSetWindowUserPointer(glfwWindow_, this);
	glfwSetWindowPosCallback(glfwWindow_, winWindowPosCallback);
	glfwSetWindowSizeCallback(glfwWindow_, winWindowSizeCallback);
	glfwSetWindowCloseCallback(glfwWindow_, winWindowCloseCallback);
	glfwSetWindowRefreshCallback(glfwWindow_, winWindowRefreshCallback);
	glfwSetWindowFocusCallback(glfwWindow_, winWindowFocusCallback);
	glfwSetWindowIconifyCallback(glfwWindow_, winWindowIconifyCallback);
	glfwSetWindowMaximizeCallback(glfwWindow_, winWindowMaximizeCallback);
	glfwSetFramebufferSizeCallback(glfwWindow_, winFramebufferSizeCallback);
	glfwSetWindowContentScaleCallback(glfwWindow_, winWindowContentScaleCallback);
	glfwSetKeyCallback(glfwWindow_, winKeyCallback);
	glfwSetCharCallback(glfwWindow_, winCharCallback);
	glfwSetCharModsCallback(glfwWindow_, winCharModsCallback);
	glfwSetMouseButtonCallback(glfwWindow_, winMouseButtonCallback);
	glfwSetCursorPosCallback(glfwWindow_, winCursorPosCallback);
	glfwSetCursorEnterCallback(glfwWindow_, winCursorEnterCallback);
	glfwSetScrollCallback(glfwWindow_, winScrollCallback);
	glfwSetJoystickCallback(winJoystickCallback);
	glfwSetDropCallback(glfwWindow_, winDropCallback);

	// Initialize input states to None (no event yet)
	keyActions_.fill(KeyAction::None);
	mouseButtonActions_.fill(MouseButtonAction::None);

	// Initialize ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
	ImGui::StyleColorsDark();

	if (std::filesystem::exists("C:\\Windows\\Fonts\\Arial.ttf")) {
		io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Arial.ttf", 16.0f);
		io.Fonts->Build();
	} else {
		std::println("Arial font not found, using default font");
	}

	ImGui_ImplGlfw_InitForOpenGL(glfwWindow_, true);
	ImGui_ImplOpenGL3_Init("#version 460");

	glViewport(0, 0, 800, 600);

	std::println("Lunatic Engine initialized successfully");
    initReflection();

	rootInstance = std::make_shared<Instance>("Root");
}

void Engine::run() {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
	while (!glfwWindowShouldClose(glfwWindow_)) {
		// Poll events first so callbacks update input state before we render the frame
		glfwPollEvents();

		glClearColor(0.1f, 0.1f, 0.125f, 1.00f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		renderer_.render(renderables_);
		//luaManager_.resume(); // Resume yielded scripts

		for (const auto& updateable : updateables_) {
			if (updateable) {
				updateable->update();
			}
		}

		if (ImGui::Begin("Renderables list")) {
			{
				ImGuiListClipper clipper;
				clipper.Begin(static_cast<int>(renderables_.size()));
				while (clipper.Step()) {
					for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
						auto* renderable = renderables_[i];
						if (renderable) {
							ImGui::Text("0x%p", static_cast<void*>(renderable));
							ImGui::SameLine();
							ImGui::PushID(renderable);
							if (ImGui::SmallButton("Copy")) {
								ImGui::SetClipboardText(std::format("0x{:X}", reinterpret_cast<uintptr_t>(renderable)).c_str());
							}
							ImGui::PopID();
						}
						else {
							ImGui::Text("Renderable: nullptr");
						}
					}
				}
			}
		}
			ImGui::End();

		// --- Instance Hierarchy and Inspector ---
		if (ImGui::Begin("Instance Hierarchy")) {
			std::function<void(const std::shared_ptr<Instance>&)> drawNode;
			drawNode = [&](const std::shared_ptr<Instance>& node) {
				if (!node) return;

				const auto& children = node->getChildren();
				bool hasChildren = !children.empty();

				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
				// If no children, make it a leaf without arrow and don't allow opening
				if (!hasChildren) {
					flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
				}

				bool isSelected = !selectedInstance_.expired() && selectedInstance_.lock().get() == node.get();
				if (isSelected) flags |= ImGuiTreeNodeFlags_Selected;

				bool open = false;
				if (hasChildren) {
					open = ImGui::TreeNodeEx(static_cast<void*>(node.get()), flags, "%s", node->getName().data());
				}
				else {
					ImGui::TreeNodeEx(static_cast<void*>(node.get()), flags, "%s", node->getName().data());
				}

				// Selection handling
				if (ImGui::IsItemClicked()) {
					selectedInstance_ = node;
					inspectorNameBuffer_ = std::string(node->getName());
				}

				// Recurse if opened
				if (hasChildren && open) {
					ImGuiListClipper clipper;
					clipper.Begin(static_cast<int>(children.size()));
					while (clipper.Step()) {
						for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
							drawNode(children[i]);
						}
					}
					ImGui::TreePop();
				}
				};

			drawNode(rootInstance);
		}
			ImGui::End();

		if (ImGui::Begin("Instance Inspector")) {
			if (auto sel = selectedInstance_.lock()) {
				ImGui::Text("Address: 0x%p", static_cast<void*>(sel.get()));
				ImGui::Text("Class: %s", std::string(sel->getClassName()).c_str());
				if (auto parent = sel->getParent()) {
					ImGui::Text("Parent: %s (0x%p)", std::string(parent->getName()).c_str(), static_cast<void*>(parent.get()));
				}
				else {
					ImGui::Text("Parent: <none>");
				}

				// Name edit
				ImGui::Separator();
				ImGui::Text("Edit");
				char buf[256] = {};
				strncpy_s(buf, inspectorNameBuffer_.c_str(), IM_ARRAYSIZE(buf));
				if (ImGui::InputText("Name", buf, IM_ARRAYSIZE(buf))) {
					inspectorNameBuffer_ = buf;
				}
				ImGui::SameLine();
				if (ImGui::SmallButton("Apply")) {
					sel->setName(inspectorNameBuffer_);
				}

				// Renderable details (if applicable)
				if (auto asRenderable = dynamic_cast<Renderable*>(sel.get())) {
					ImGui::Separator();
					ImGui::Text("Renderable");
					ImGui::Text("Registered: %s", (
						[&]() {
							return std::find(renderables_.begin(), renderables_.end(), asRenderable) != renderables_.end();
						}() ? "Yes" : "No"));

					ImGui::DragFloat2("Position", &asRenderable->position.x, 0.025f);
					ImGui::DragFloat2("Scale", &asRenderable->scale.x, 0.025f);

					// Sprite-specific info
					if (std::string(sel->getClassName()) == "Sprite") {
						ImGui::Text("Type: Sprite");
					}
				}

				// Children summary
				ImGui::Separator();
				const auto& kids = sel->getChildren();
				ImGui::Text("Children: %d", static_cast<int>(kids.size()));
				{
					ImGuiListClipper clipper;
					clipper.Begin(static_cast<int>(kids.size()));
					while (clipper.Step()) {
						for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
							const auto& c = kids[i];
							ImGui::BulletText("%s (0x%p)", std::string(c->getName()).c_str(), static_cast<void*>(c.get()));
						}
					}
				}
			}
			else {
				ImGui::Text("No selection");
			}
		}
			ImGui::End();

		renderStats(ImGui::GetIO().DeltaTime, static_cast<int>(renderables_.size()));

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(glfwWindow_);
	}
}

void Engine::hideCursor() {
	if (glfwWindow_) {
		glfwSetInputMode(glfwWindow_, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	}
}

void Engine::showCursor() {
	if (glfwWindow_) {
		glfwSetInputMode(glfwWindow_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
}

// uses imgui fg draw list
void Engine::drawText(glm::vec2 position, const std::string& text, glm::vec4 color) {
	glm::vec2 screenPos = renderer_.getMainCamera()->worldToScreen(position);

	// Calculate text size to center it
	ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
	ImVec2 textPos = ImVec2(screenPos.x - textSize.x * 0.5f, screenPos.y - textSize.y * 0.5f);
	ImGui::GetForegroundDrawList()->AddText(
		textPos,
		ImColor(color.r, color.g, color.b, color.a),
		text.c_str()
	);

	/*
	ImGui::GetForegroundDrawList()->AddText(
		ImVec2(screenPos.x, screenPos.y),
		ImColor(color.r, color.g, color.b, color.a),
		text.c_str()
	);
	*/
}

void Engine::resize(int width, int height) {
	if (glfwWindow_) {
		glViewport(0, 0, width, height);
		renderer_.resize(width, height);
	}
}

Engine::~Engine() {
	if (glfwWindow_) glfwDestroyWindow(glfwWindow_);
	glfwTerminate();
}

#define W2Engine(window) reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window))

void Engine::winErrorCallback(int error_code, const char* description){
	std::fprintf(stderr, "GLFW Error %d: %s\n", error_code, description);
}

void Engine::winWindowPosCallback(GLFWwindow* window, int xpos, int ypos){
	//std::print("Window position changed to ({}, {})\n", xpos, ypos);
	Engine* engine = W2Engine(window);
	if (engine) {
		engine->windowPosX_ = xpos;
		engine->windowPosY_ = ypos;
	}
}

void Engine::winWindowSizeCallback(GLFWwindow* window, int width, int height){
	//std::print("Window size changed to {}x{}\n", width, height);
	Engine* engine = W2Engine(window);
	if (engine) {
		engine->windowWidth_ = width;
		engine->windowHeight_ = height;
		glViewport(0, 0, width, height);
	}
}

void Engine::winWindowCloseCallback(GLFWwindow* window){
	//std::print("Window close requested\n");
	Engine* engine = W2Engine(window);
	if (engine) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
}

void Engine::winWindowRefreshCallback(GLFWwindow* window){
	//std::print("Window refresh requested\n");
	Engine* engine = W2Engine(window);
	if (engine) {
		// This should never be necessary... but we can call it to ensure the window is redrawn.
		glfwSwapBuffers(window);
	}
}

void Engine::winWindowFocusCallback(GLFWwindow* window, int focused){
	//std::print("Window focus changed: {}\n", focused ? "focused" : "unfocused");
	Engine* engine = W2Engine(window);
	if (engine) {
		engine->mouseInWindow_ = focused;
	}
}

void Engine::winWindowIconifyCallback(GLFWwindow* window, int iconified){
	//std::print("Window iconified state changed: {}\n", iconified ? "iconified" : "restored");
	Engine* engine = W2Engine(window);
	if (engine) {
		// Handle iconification if needed, e.g., pause updates or animations.
		engine->mouseInWindow_ = !iconified; // Update mouseInWindow state
	}
}

void Engine::winWindowMaximizeCallback(GLFWwindow* window, int maximized){
	//std::print("Window maximized state changed: {}\n", maximized ? "maximized" : "restored");
	// Unhandled for now
}

void Engine::winFramebufferSizeCallback(GLFWwindow* window, int width, int height){
	//std::print("Framebuffer size changed to {}x{}\n", width, height);
	Engine* engine = W2Engine(window);
	if (engine) {
		engine->windowWidth_ = width;
		engine->windowHeight_ = height;
		glViewport(0, 0, width, height);
		engine->renderer_.resize(width, height);
	}
}

void Engine::winWindowContentScaleCallback(GLFWwindow* window, float xscale, float yscale){
	//std::print("Window content scale changed to ({}, {})\n", xscale, yscale);
	Engine* engine = W2Engine(window);
	if (engine) {
		// Handle content scale changes if needed, e.g., for high DPI displays.
		// This is not currently used in the engine, but could be useful for scaling UI elements.
	}
}

void Engine::winKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods){
	//std::print("Key event: key={}, scancode={}, action={}, mods={}\n", key, scancode, action, mods);
	Engine* engine = W2Engine(window);
	if (engine) {
		if (key >= 0 && key < engine->keyActions_.size()) {
			switch (action) {
			case GLFW_PRESS:
				engine->keyActions_[key] = KeyAction::Pressed;
				break;
			case GLFW_RELEASE:
				engine->keyActions_[key] = KeyAction::Released;
				break;
			case GLFW_REPEAT:
				// Ignore repeat; let getters transition Pressed->Held
				break;
			default:
				break;
			}
		}
		else {
			std::fprintf(stderr, "Key out of range: %d\n", key);
		}
	}
}

void Engine::winCharCallback(GLFWwindow* window, unsigned int codepoint){
	//std::print("Character input: codepoint={}\n", codepoint);
	Engine* engine = W2Engine(window);
	if (engine) {
		// Handle character input, e.g., for text input fields.
		// This is not currently used in the engine, but could be useful for text input handling.
		// std::print("Character input received: {}\n", static_cast<char>(codepoint));
	}
}

void Engine::winCharModsCallback(GLFWwindow* window, unsigned int codepoint, int mods){
	//std::print("Character input with modifiers: codepoint={}, mods={}\n", codepoint, mods);
	Engine* engine = W2Engine(window);
	if (engine) {
		// Handle character input with modifiers, e.g., for text input fields with Ctrl/Cmd or Shift.
		// This is not currently used in the engine, but could be useful for text input handling.
		// std::print("Character input with modifiers received: {} (mods: {})\n", static_cast<char>(codepoint), mods);
	}
}

void Engine::winMouseButtonCallback(GLFWwindow* window, int button, int action, int mods){
	//std::print("Mouse button event: button={}, action={}, mods={}\n", button, action, mods);
	Engine* engine = W2Engine(window);
	if (engine) {
		if (button >= 0 && button < engine->mouseButtonActions_.size()) {
			switch (action) {
			case GLFW_PRESS:
				engine->mouseButtonActions_[button] = MouseButtonAction::Pressed;
				break;
			case GLFW_RELEASE:
				engine->mouseButtonActions_[button] = MouseButtonAction::Released;
				break;
			case GLFW_REPEAT:
				// Ignore repeat for mouse buttons as well
				break;
			default:
				break;
			}
		}
		else {
			std::fprintf(stderr, "Mouse button out of range: %d\n", button);
		}
	}
}

void Engine::winCursorPosCallback(GLFWwindow* window, double xpos, double ypos){
	//std::print("Cursor position changed to ({}, {})\n", xpos, ypos);
	Engine* engine = W2Engine(window);
	if (engine) {
		// Compute delta relative to previous stored position
		if (engine->mouseInWindow_) {
			engine->mouseDeltaX_ = xpos - engine->prevMouseX_;
			engine->mouseDeltaY_ = ypos - engine->prevMouseY_;
		}
		else {
			engine->mouseDeltaX_ = 0.0;
			engine->mouseDeltaY_ = 0.0;
		}

		engine->mouseX_ = xpos;
		engine->mouseY_ = ypos;
		engine->prevMouseX_ = xpos;
		engine->prevMouseY_ = ypos;
	}
}

void Engine::winCursorEnterCallback(GLFWwindow* window, int entered){
	//std::print("Cursor entered window: {}\n", entered ? "entered" : "left");
	Engine* engine = W2Engine(window);
	if (engine) {
		engine->mouseInWindow_ = entered;
		if (!entered) {
			engine->mouseDeltaX_ = 0.0;
			engine->mouseDeltaY_ = 0.0;
		}
	}
}

void Engine::winScrollCallback(GLFWwindow* window, double xoffset, double yoffset){
	//std::print("Scroll event: xoffset={}, yoffset={}\n", xoffset, yoffset);
	Engine* engine = W2Engine(window);
	if (engine) {
		engine->mouseScrollX_ += xoffset;
		engine->mouseScrollY_ += yoffset;
		engine->mouseScrollDeltaX_ = xoffset;
		engine->mouseScrollDeltaY_ = yoffset;
	}
}

KeyAction Engine::getKeyState(int key) {
	if (key < 0 || key >= static_cast<int>(keyActions_.size())) return KeyAction::None;
	KeyAction state = keyActions_[key];
	if (state == KeyAction::Pressed) {
		keyActions_[key] = KeyAction::Held;
	} else if (state == KeyAction::Released) {
		keyActions_[key] = KeyAction::None;
	}
	return state;
}

MouseButtonAction Engine::getMouseButtonState(int button) {
	if (button < 0 || button >= static_cast<int>(mouseButtonActions_.size())) return MouseButtonAction::None;
	MouseButtonAction state = mouseButtonActions_[button];
	if (state == MouseButtonAction::Pressed) {
		mouseButtonActions_[button] = MouseButtonAction::Held;
	} else if (state == MouseButtonAction::Released) {
		mouseButtonActions_[button] = MouseButtonAction::None;
	}
	return state;
}

double Engine::getMouseX() const { return mouseX_; }
double Engine::getMouseY() const { return mouseY_; }
glm::vec2 Engine::getMousePos() const {
	float x = static_cast<float>(mouseX_);
	float y = static_cast<float>(mouseY_);

	return { x, y };
}

double Engine::getMouseDeltaX() {
	double d = mouseDeltaX_;
	mouseDeltaX_ = 0.0;
	return d;
}

double Engine::getMouseDeltaY() {
	double d = mouseDeltaY_;
	mouseDeltaY_ = 0.0;
	return d;
}

double Engine::getScrollDeltaX() {
	double d = mouseScrollDeltaX_;
	mouseScrollDeltaX_ = 0.0;
	return d;
}

double Engine::getScrollDeltaY() {
	double d = mouseScrollDeltaY_;
	mouseScrollDeltaY_ = 0.0;
	return d;
}

void Engine::winJoystickCallback(int jid, int event){
	// For now, we don't handle joystick events in the engine.
}

void Engine::winDropCallback(GLFWwindow* window, int path_count, const char* paths[]){
	// For now, we don't handle file drop events in the engine.
	/*std::fprintf(stderr, "File drop event: %d paths dropped\n", path_count);
	for (int i = 0; i < path_count; ++i) {
		std::fprintf(stderr, "Path %d: %s\n", i, paths[i]);
	}*/
}

// TODO: We should sort the renderables by zIndex
// This should be done with a double buffered renderable list
// It should have an atomic reference which switches between the two lists
void Engine::registerRenderable(Renderable* renderable) {
    if (std::find(renderables_.begin(), renderables_.end(), renderable) == renderables_.end()) {
        renderables_.push_back(renderable);
    } else {
		std::println("Renderable already registered: {:X}", reinterpret_cast<uintptr_t>(renderable));
	}
}

void Engine::unregisterRenderable(Renderable* renderable) {
    auto it = std::find(renderables_.begin(), renderables_.end(), renderable);
    if (it != renderables_.end()) {
        std::iter_swap(it, renderables_.end() - 1);
        renderables_.pop_back();
    }
}

void Engine::registerUpdateable(Updateable* updateable) {
	if (std::find(updateables_.begin(), updateables_.end(), updateable) == updateables_.end()) {
		updateables_.push_back(updateable);
	} else {
		std::println("Updateable already registered: {:X}", reinterpret_cast<uintptr_t>(updateable));
	}
}

void Engine::unregisterUpdateable(Updateable* updateable) {
	auto it = std::find(updateables_.begin(), updateables_.end(), updateable);
	if (it != updateables_.end()) {
		std::iter_swap(it, updateables_.end() - 1);
		updateables_.pop_back();
	}
}

void Engine::registerMainCamera(Camera* camera) {
	renderer_.registerMainCamera(camera);
}
