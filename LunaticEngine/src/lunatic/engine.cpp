#include "pch.h"
#include "lunatic/engine.h"

Lunatic::Engine* Lunatic::Engine::s_instance = nullptr;
Lunatic::Engine::Engine(std::uint32_t width, std::uint32_t height, std::string_view title)
	: m_windowSize(width, height), m_mousePos(0.0f, 0.0f) {
	LUN_ASSERT(s_instance == nullptr, "Engine instance already exists, did you forget to destroy it?");
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
		engine->m_scene->camera.resize(width, height);
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
#endif

	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(m_window, true);
	ImGui_ImplOpenGL3_Init("#version 460");

	m_scene = std::make_shared<Scene>();
}

void reparentObject(std::shared_ptr<Lunatic::Object> obj, std::shared_ptr<Lunatic::Object> newParent);
bool isChild(Lunatic::Object* potentialChild, Lunatic::Object* parent);

void Lunatic::Engine::debugInspector() {
    ImGui::Begin("Inspector");

    // Helper to find the shared_ptr for any raw Object* in our scene
    auto findSP = [&](Lunatic::Object* raw) {
        std::function<std::shared_ptr<Lunatic::Object>(const std::vector<std::shared_ptr<Lunatic::Object>>&)> recurse;
        recurse = [&](auto const& list) -> std::shared_ptr<Lunatic::Object> {
            for (auto const& sp : list) {
                if (sp.get() == raw) return sp;
                if (auto found = recurse(sp->children))
                    return found;
            }
            return nullptr;
        };
        return recurse(m_scene->objects);
    };

    static Lunatic::Object* selectedObject = nullptr;
    static std::shared_ptr<Lunatic::Object> newObject = nullptr;

    if (ImGui::BeginChild("SceneHierarchy", ImVec2(250, 0), true)) {
        ImGui::Text("Scene Hierarchy");
        ImGui::Separator();

        std::function<void(Lunatic::Object*, int)> drawNode;
        drawNode = [&](Lunatic::Object* obj, int depth) {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
            if (obj == selectedObject) flags |= ImGuiTreeNodeFlags_Selected;
            if (obj->children.empty()) flags |= ImGuiTreeNodeFlags_Leaf;

            bool open = ImGui::TreeNodeEx((void*)obj, flags, "%s", obj->name);

            // Handle selection
            if (ImGui::IsItemClicked())
                selectedObject = obj;

            // Drag source on the tree node item
            if (ImGui::IsItemActive() && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                ImGui::SetDragDropPayload("ObjectPayload", &obj, sizeof(obj));
                ImGui::Text("%s", obj->name);
                ImGui::EndDragDropSource();
            }

            // Drop target
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ObjectPayload")) {
                    auto droppedRaw = *(Lunatic::Object**)payload->Data;
                    if (droppedRaw != obj && !isChild(droppedRaw, obj)) {
                        if (auto droppedSP = findSP(droppedRaw)) {
                            if (auto parentSP = findSP(obj)) {
                                reparentObject(droppedSP, parentSP);
                            }
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }

            if (open) {
                for (const auto& child : obj->children)
                    drawNode(child.get(), depth + 1);
                ImGui::TreePop();
            }
        };

        for (const auto& root : m_scene->objects) {
            if (root->parent.expired())
                drawNode(root.get(), 0);
        }
        ImGui::EndChild();
    }

    ImGui::SameLine();
    ImGui::BeginChild("ObjectInspector", ImVec2(0, 0), true);

    ImGui::Text("Object Inspector");
    ImGui::Separator();

    if (selectedObject) {
        ImGui::PushID(selectedObject);

        if (ImGui::CollapsingHeader("Basic Info", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::InputText("Name", selectedObject->name, sizeof(selectedObject->name));
            ImGui::ColorEdit4("Color", (float*)&selectedObject->color);
        }

        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloat3("Position", (float*)&selectedObject->position, 0.1f);
            ImGui::DragFloat2("Scale", (float*)&selectedObject->scale, 0.05f, 0.01f, 100.0f);
            ImGui::DragFloat("Rotation", &selectedObject->rotation, 1.0f, 0.0f, 360.0f, "%.1f");
        }

        if (ImGui::CollapsingHeader("Hierarchy")) {
            auto pSP = selectedObject->parent.lock();
            if (pSP)
                ImGui::Text("Parent: %s", pSP->name);
            else
                ImGui::Text("Parent: None");

            if (!selectedObject->children.empty()) {
                ImGui::Text("Children:");
                for (auto& childSP : selectedObject->children) {
                    ImGui::BulletText("%s", childSP->name);
                    // Drag source for children
                    if (ImGui::IsItemActive() && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                        auto raw = childSP.get();
                        ImGui::SetDragDropPayload("ObjectPayload", &raw, sizeof(raw));
                        ImGui::Text("%s", childSP->name);
                        ImGui::EndDragDropSource();
                    }
                }
            } else {
                ImGui::Text("No children");
            }

            if (ImGui::Button("Make Root") && pSP) {
                pSP->detachChild(findSP(selectedObject));
            }
        }

        ImGui::PopID();
    } else {
        ImGui::Text("Select an object to inspect.");
    }

    if (ImGui::Button("Add New Object")) {
        newObject = std::make_shared<Lunatic::Object>();
        newObject->position = glm::vec3(0.0f);
        newObject->scale    = glm::vec2(1.0f);
        newObject->rotation = 0.0f;
        m_scene->objects.push_back(newObject);
    }

    ImGui::EndChild();
    ImGui::End();
}

void reparentObject(std::shared_ptr<Lunatic::Object> obj, std::shared_ptr<Lunatic::Object> newParent) {
    // Remove obj from its current parent (if any)
    if (auto oldParent = obj->parent.lock()) {
        oldParent->detachChild(obj);
    }

    // Add obj to the new parent
    newParent->attachChild(obj);
}

bool isChild(Lunatic::Object* potentialChild, Lunatic::Object* parent) {
    if (potentialChild == parent) {
        return true; // Avoid self-parenting
    }

    // Traverse upwards to check if parent is an ancestor
    while (auto p = potentialChild->parent.lock()) {
        if (p.get() == parent) {
            return true;
        }
        potentialChild = p.get();
    }
    return false;
}

void Lunatic::Engine::drawObjectRecursive(const std::shared_ptr<Lunatic::Object>& obj, const glm::mat4& parentModel) {
    // Calculate local transform
	glm::mat4 model = glm::translate(parentModel, glm::vec3(obj->position.x, obj->position.y, 0.0f));
    model = glm::rotate(model, -glm::radians(obj->rotation), { 0.0f, 0.0f, 1.0f });
    model = glm::scale(model, { obj->scale.x, obj->scale.y, 1.0f });

    m_scene->shader.set("u_model", model);
    m_scene->shader.set("u_colour", obj->color);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    for (const auto& child : obj->children) {
        drawObjectRecursive(child, model);
    }
}

void Lunatic::Engine::run() {
	m_scene->camera.resize((int)m_windowSize.x, (int)m_windowSize.y);

	m_running = true;
	while (!glfwWindowShouldClose(m_window)) {
		glfwPollEvents();

		glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		debugInspector();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		m_scene->shader.use();
		m_scene->camera.update();
		m_scene->shader.set("u_viewProjection", m_scene->camera.getVP());
		m_scene->rectBuffers.bind();
		for (const auto& root : m_scene->objects) {
			if (root->parent.expired()) {
				drawObjectRecursive(root, glm::mat4(1.0f));
			}
		}

		glfwSwapBuffers(m_window);
	}
	m_running = false;
}

Lunatic::Engine::~Engine() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwDestroyWindow(m_window);
	glfwTerminate();

	s_instance = nullptr;
}