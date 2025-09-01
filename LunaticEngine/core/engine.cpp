#include "pch.h"

#include "engine.h"
#include "render/renderer.h"

#include "model/primitives/physicssprite.h"

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
	ImGui::Text("Average Frame Time: %.2f ms", std::accumulate(frameTimes.data(), frameTimes.data() + frameTimes.size(), 0.0f) / frameTimes.size());
	ImGui::Text("Average FPS: %.1f", std::accumulate(fpsValues.data(), fpsValues.data() + fpsValues.size(), 0.0f) / fpsValues.size());
	ImGui::Text("Average FPS (imgui reports): %.1f", ImGui::GetIO().Framerate);
	ImGui::Text("Instance Count: %d", renderableCount);

	ImGui::Separator();

	// Manipulation of hierarchy stuff
	if (ImGui::Button("New Instance")) {
		auto newInstance = std::make_shared<Instance>("New Instance");
		newInstance->setParent(Engine::getInstance().rootInstance);
	}

    ImGui::End();
}

void Engine::init(GLFWwindow* externalWindow, bool initLibs) {
	if (glfwWindow_) {
		spdlog::warn("Engine already initialized");
		return;
	}

	if (externalWindow) {
		glfwWindow_ = externalWindow;
		spdlog::info("Using external GLFW window");
	}
	else {
		if (!glfwInit()) {
			spdlog::critical("Failed to initialize GLFW");
			return;
		}

		glfwWindowHint(GLFW_SAMPLES, 4);  

		glfwSetErrorCallback(winErrorCallback);
		glfwWindow_ = glfwCreateWindow(windowWidth_, windowHeight_, windowTitle_, nullptr, nullptr);
		glfwMakeContextCurrent(glfwWindow_);

		if (!glfwWindow_) {
			spdlog::critical("Engine initialization failed due to window creation failure");
			return;
		}
		spdlog::info("Created new GLFW window");
	}

	glfwSwapInterval(1);
	glfwMakeContextCurrent(glfwWindow_);

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

	if (initLibs) {
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
			spdlog::critical("Failed to initialize GLAD");
			return;
		}
		glEnable(GL_MULTISAMPLE);

		// Initialize ImGui
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;

		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
		ImGui::StyleColorsDark();

		if (std::filesystem::exists("C:\\Windows\\Fonts\\Arial.ttf")) {
			io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Arial.ttf", 16.0f);
			io.Fonts->Build();
		}
		else {
			spdlog::warn("Arial font not found, using default font");
		}

		ImGui_ImplGlfw_InitForOpenGL(glfwWindow_, true);
		ImGui_ImplOpenGL3_Init("#version 460");
	}

	glViewport(0, 0, 800, 600);
}

Engine::Engine() {
	// Initialize input states to None (no event yet)
	keyActions_.fill(KeyAction::None);
	mouseButtonActions_.fill(MouseButtonAction::None);

	rootInstance = std::make_shared<Instance>("Root");
	spdlog::info("Lunatic Engine initialized successfully");
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

		// Show FPS in top left (draw list, green text)
		auto dl = ImGui::GetForegroundDrawList();
		dl->AddText(ImVec2(10, 10), IM_COL32(0, 255, 0, 255), std::format("FPS: {:.1f}", ImGui::GetIO().Framerate).c_str());

		renderer_.render(renderables_);
		//luaManager_.resume(); // Resume yielded scripts

		for (const auto& updateable : updateables_)
			if (updateable)
				updateable->update();
		PhysicsSprite::stepAll(1.0f / 60.0f); // Fixed timestep for physics (we should fix this)

		/*
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

		if (ImGui::Begin("Updateables list")) {
			{
				ImGuiListClipper clipper;
				clipper.Begin(static_cast<int>(updateables_.size()));
				while (clipper.Step()) {
					for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
						auto* updateable = updateables_[i];
						if (updateable) {
							ImGui::Text("0x%p", static_cast<void*>(updateable));
							ImGui::SameLine();
							ImGui::PushID(updateable);
							if (ImGui::SmallButton("Copy")) {
								ImGui::SetClipboardText(std::format("0x{:X}", reinterpret_cast<uintptr_t>(updateable)).c_str());
							}
							ImGui::PopID();
						}
						else {
							ImGui::Text("Updateable: nullptr");
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

				ImGui::Separator();

				auto& ti = sel->typeInfo;
				auto props = ti.get_properties();
				ImGui::Text("Properties: %d", static_cast<int>(props.size()));
				{
					for (const auto& prop : props) {
						auto name = prop.get_name().to_string();
						auto type = prop.get_type();
						auto value = prop.get_value(sel);
						if (!value) continue; // Skip invalid values

						ImGui::PushID(name.c_str());
						ImGui::BeginDisabled(prop.is_readonly());

						if (type.is_arithmetic()) {
							if (type == rttr::type::get<int>()) {
								int v = value.to_int();
								if (ImGui::InputInt(name.c_str(), &v)) {
									prop.set_value(sel, v);
								}
							}
							else if (type == rttr::type::get<float>()) {
								float v = value.to_float();
								if (ImGui::InputFloat(name.c_str(), &v)) {
									prop.set_value(sel, v);
								}
							}
							else if (type == rttr::type::get<double>()) {
								double v = value.to_double();
								if (ImGui::InputDouble(name.c_str(), &v)) {
									prop.set_value(sel, v);
								}
							}
							else if (type == rttr::type::get<bool>()) {
								bool v = value.to_bool();
								if (ImGui::Checkbox(name.c_str(), &v)) {
									prop.set_value(sel, v);
								}
							}
							else {
								ImGui::Text("%s: <unhandled arithmetic type>", name.c_str());
							}
						}
						else if (type.is_class()) {
							if (type == rttr::type::get<std::string>()) {
								std::string v = value.to_string();
								char strBuf[256];
								strncpy_s(strBuf, v.c_str(), sizeof(strBuf));
								if (ImGui::InputText(name.c_str(), strBuf, sizeof(strBuf))) {
									prop.set_value(sel, std::string(strBuf));
								}
							}
							else if (type == rttr::type::get<std::string_view>()) {
								std::string v = value.to_string();
								char strBuf[256];
								strncpy_s(strBuf, v.c_str(), sizeof(strBuf));
								if (ImGui::InputText(name.c_str(), strBuf, sizeof(strBuf))) {
									std::string_view sv(strBuf);
									prop.set_value(sel, sv);
								}
							} else if (type == rttr::type::get<glm::vec2>()) {
								glm::vec2 v = value.get_value<glm::vec2>();
								if (ImGui::InputFloat2(name.c_str(), &v.x)) {
									prop.set_value(sel, v);
								}
							}
							else if (type == rttr::type::get<glm::vec3>()) {
								glm::vec3 v = value.get_value<glm::vec3>();
								if (ImGui::InputFloat3(name.c_str(), &v.x)) {
									prop.set_value(sel, v);
								}
							}
							else if (type == rttr::type::get<glm::vec4>()) {
								glm::vec4 v = value.get_value<glm::vec4>();
								if (ImGui::InputFloat4(name.c_str(), &v.x)) {
									prop.set_value(sel, v);
								}
							}
							else if (type == rttr::type::get<glm::quat>()) {
								glm::quat v = value.get_value<glm::quat>();
								float euler[3] = {};
								// Convert quat to euler angles for editing
								glm::vec3 eulerVec = glm::eulerAngles(v);
								euler[0] = glm::degrees(eulerVec.x);
								euler[1] = glm::degrees(eulerVec.y);
								euler[2] = glm::degrees(eulerVec.z);
								if (ImGui::InputFloat3(name.c_str(), euler)) {
									// Convert back to quat
									glm::vec3 newEuler = glm::radians(glm::vec3(euler[0], euler[1], euler[2]));
									glm::quat newQuat = glm::quat(newEuler);
									prop.set_value(sel, newQuat);
								}
							}
							else if (type == rttr::type::get<rttr::variant>()) {
								ImGui::Text("%s: <variant>", name.c_str());
							}
							else if (type == rttr::type::get<std::shared_ptr<Instance>>()) {
								auto inst = value.get_value<std::shared_ptr<Instance>>();
								if (inst) {
									ImGui::Text("%s: %s (0x%p)", name.c_str(), std::string(inst->getName()).c_str(), static_cast<void*>(inst.get()));
								}
								else {
									ImGui::Text("%s: <null>", name.c_str());
								}
							}
							else {
								ImGui::Text("%s: <unhandled class type>", name.c_str());
							}
						}
						else if (type.is_pointer()) {
							ImGui::Text("%s: 0x%p", name.c_str(), static_cast<void*>(value.get_value<std::shared_ptr<Instance>>().get()));
						}
						else {
							ImGui::Text("%s: <unhandled type %s>", name.c_str(), type.get_name().to_string().c_str());
						}

						ImGui::EndDisabled();
						ImGui::PopID();
					}
				}

				ImGui::Separator();

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

		renderStats(ImGui::GetIO().DeltaTime, static_cast<int>(renderables_.size()));*/

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(glfwWindow_);
	}
}

void Engine::hideCursor() {
	int vx, vy, vw, vh;
	renderer_.getViewport(vx, vy, vw, vh);

	if (mouseX_ < vx || mouseX_ > (vx + vw) || mouseY_ < vy || mouseY_ > (vy + vh)) {
		return;
	}

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

	// Apply viewport offset
	int vx, vy, vw, vh;
	renderer_.getViewport(vx, vy, vw, vh);
	textPos.x += vx;
	textPos.y += vy;

	ImGui::GetForegroundDrawList()->AddText(
		textPos,
		ImColor(color.r, color.g, color.b, color.a),
		text.c_str()
	);
}

void Engine::resize(int width, int height) {
	if (!autoResizeRenderer_) return;

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
	Engine* engine = W2Engine(window);
	if (!engine->autoResizeRenderer_) return;

	if (engine) {
		engine->windowWidth_ = width;
		engine->windowHeight_ = height;
		glViewport(0, 0, width, height);
		engine->renderer_.resize(width, height);
		engine->renderer_.setViewport(0, 0, width, height);
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
	/*float x = static_cast<float>(mouseX_);
	float y = static_cast<float>(mouseY_);

	return { x, y };*/

	// used to be return { x, y }.
	// HOWEVER, we now have a "viewport" in the renderer which may not be full window size
	// So we need to convert mouse position to viewport space (not NDC, just viewport pixels)
	int vx, vy, vw, vh;
	renderer_.getViewport(vx, vy, vw, vh);
	float x = static_cast<float>(mouseX_ - vx);
	float y = static_cast<float>(mouseY_ - vy);
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
		spdlog::warn("Renderable already registered: {:X}", reinterpret_cast<uintptr_t>(renderable));
	}
}

void Engine::unregisterRenderable(Renderable* renderable) {
    auto it = std::find(renderables_.begin(), renderables_.end(), renderable);
    if (it != renderables_.end()) {
        std::iter_swap(it, renderables_.end() - 1);
        renderables_.pop_back();
	} else {
		//spdlog::trace("Tried to unregister non-registered Renderable {:X}", reinterpret_cast<uintptr_t>(renderable));
    }
}

void Engine::registerUpdateable(Updateable* updateable) {
	if (std::find(updateables_.begin(), updateables_.end(), updateable) == updateables_.end()) {
		updateables_.push_back(updateable);
	} else {
		spdlog::warn("Updateable already registered: {:X}", reinterpret_cast<uintptr_t>(updateable));
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
