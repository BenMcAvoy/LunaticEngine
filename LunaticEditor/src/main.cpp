#include "pch.h"
#include "window.h"
#include <unordered_map>
#include <array>

#include "core/engine.h"
#include "render/framebuffer.h"
#include "model/primitives/sprite.h"
#include "model/primitives/physicssprite.h"

template <typename T>
std::shared_ptr<T> AddObjectTo(std::string name, std::shared_ptr<Lunatic::Instance> parent) {
    auto inst = std::make_shared<T>(std::move(name));
    inst->setParent(parent);
    return inst;
}

int main(int argc, char** argv) {
    GLFWwindow* window = createWindow(1280, 720, "Lunatic Editor");

	// Change CWD to ../LunaticRuntime if it exists
    if (std::filesystem::exists("../LunaticRuntime")) {
        std::filesystem::current_path("../LunaticRuntime");
    }

	Lunatic::Engine& engine = Lunatic::Engine::getInstance();
	engine.init(window, false);
	engine.setAutoResizeRenderer(false);
	auto& renderer = engine.getRenderer();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460");

	auto path = "res/scene.json";
	std::ifstream inFile(path);
	nlohmann::json sceneJson;
    inFile >> sceneJson;
	engine.rootInstance->deserialize(sceneJson);
	inFile.close();

	Lunatic::Framebuffer framebuffer;

    bool isPlaying = false;

	bool showViewportWindow = true;
	bool showHierarchyWindow = true;
	bool showInspectorWindow = true;
	bool showConsoleWindow = true;

    bool shouldOpenHelp = false;
	bool shouldOpenNewInstance = false;

    std::weak_ptr<Lunatic::Instance> selectedInstance;

    renderWith([&](GLFWwindow*) {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags host_window_flags = ImGuiWindowFlags_NoTitleBar
                                           | ImGuiWindowFlags_NoCollapse
                                           | ImGuiWindowFlags_NoResize
                                           | ImGuiWindowFlags_NoMove
                                           | ImGuiWindowFlags_NoBringToFrontOnFocus
                                           | ImGuiWindowFlags_NoNavFocus
                                           | ImGuiWindowFlags_MenuBar;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::Begin("Lunatic Dockspace Host", nullptr, host_window_flags);
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Save")) {
					// Save to the same path as we loaded from
                    auto json = engine.rootInstance->serialize();
					std::ofstream(path) << json.dump(4);
					spdlog::info("Scene saved to scene.json");
                }

                if (ImGui::MenuItem("Exit")) {
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
				ImGui::MenuItem("Viewport", nullptr, &showViewportWindow);
				ImGui::MenuItem("Hierarchy", nullptr, &showHierarchyWindow);
				ImGui::MenuItem("Inspector", nullptr, &showInspectorWindow);
				ImGui::MenuItem("Console", nullptr, &showConsoleWindow);
                ImGui::EndMenu();
            }

            // If we're playing, this menu tab should have a slight highlight
            bool wasPlaying = isPlaying;
            if (wasPlaying) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.9f, 0.4f, 1.0f));

            if (ImGui::BeginMenu("Game")) {
				auto text = isPlaying ? "Stop" : "Play";
                if (ImGui::MenuItem(text)) {
                    isPlaying = !isPlaying;

                    if (isPlaying) {
                        // save the scene to tempScene.json
                        auto json = engine.rootInstance->serialize();
                        std::ofstream outFile("tempScene.json");
                        outFile << json.dump(4);
                    }
                    else {
                        // ensure no stale pointers remain in engine registries
                        auto& upd = engine.getUpdateables();
                        auto& rnd = engine.getRenderables();
                        upd.clear();
                        rnd.clear();
						// clear the current scene
						engine.rootInstance->clearChildren();

                        // load the scene from tempScene.json
                        std::ifstream inFile("tempScene.json");
                        nlohmann::json sceneJson;
                        inFile >> sceneJson;
                        engine.rootInstance->deserialize(sceneJson);
                        inFile.close();
                    }
                }
				ImGui::EndMenu();
            }
            if (wasPlaying) ImGui::PopStyleColor();
            
            if (ImGui::BeginMenu("Instance") ) {
                if (ImGui::MenuItem("New")) {
                    shouldOpenNewInstance = true;
                }

                ImGui::EndMenu();
			}

            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("About")) {
					shouldOpenHelp = true;
                }
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
		}

        if (ImGui::BeginPopupModal("About Lunatic Editor", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextWrapped("A 2D game engine crafted for seamless development, combining modern rendering with Lua scripting to deliver dynamic and fluid workflows");
            ImGui::TextLinkOpenURL("GitHub", "https://github.com/benMcAvoy/LunaticEngine/");
            ImGui::Separator();
            if (ImGui::Button("OK", ImVec2(320, 0))) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
		}

        if (ImGui::BeginPopupModal("Add New Instance", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            // get all types derived from Lunatic::Instance
            static std::vector<std::string> instanceNames;
            static std::vector<rttr::type> instanceTypes;
            static int selectedTypeIndex = 0;

            if (instanceNames.empty()) {
                auto allTypes = rttr::type::get_types();
                for (auto& t : allTypes) {
                    // if it is exactly Lunatic::Instance, skip (or include if you want)
                    if (t == rttr::type::get<Lunatic::Instance>()) {
                        instanceNames.emplace_back(t.get_name().to_string());
                        instanceTypes.emplace_back(t);
                        continue;
                    }

                    // check if it derives from Lunatic::Instance
                    auto bases = t.get_base_classes();
                    for (auto& b : bases) {
                        if (b == rttr::type::get<Lunatic::Instance>()) {
                            instanceNames.push_back(t.get_name().to_string());
                            instanceTypes.emplace_back(t);
                            break;
                        }
                    }
                }
            }

            // safely get the selected name
            const char* selectedName = instanceNames.empty() ? "" : instanceNames[selectedTypeIndex].c_str();

            if (ImGui::BeginCombo("Instance type", selectedName)) {
                for (int idx = 0; idx < instanceNames.size(); ++idx) {
                    bool is_selected = (selectedTypeIndex == idx);
                    if (ImGui::Selectable(instanceNames[idx].c_str(), is_selected))
                        selectedTypeIndex = idx;  // update selection
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

			static char nameBuf[128] = "New Instance";
			ImGui::InputText("Name", nameBuf, sizeof(nameBuf));
            if (ImGui::Button("Create", ImVec2(320, 0))) {
                // Construct a new one using the construct method
				auto& t = instanceTypes[selectedTypeIndex];

				auto method = t.get_method("construct");

				std::string_view nameSV(nameBuf);
                rttr::variant obj = method.invoke({}, nameSV);
				std::shared_ptr<Lunatic::Instance> i = obj.get_value<std::shared_ptr<Lunatic::Instance>>();

				// Set the parent to the selected instance, or root if none selected
                if (auto sel = selectedInstance.lock()) {
                    i->setParent(sel);
                } else {
                    i->setParent(engine.rootInstance);
				}

				// Select the new instance
				selectedInstance = i;

				// We are done :tada:
                // clear the name buffer for next time
                strcpy_s(nameBuf, "New Instance");
                ImGui::CloseCurrentPopup();
			}

            ImGui::EndPopup();
        }

        if (shouldOpenHelp) {
            ImGui::OpenPopup("About Lunatic Editor");
            shouldOpenHelp = false;
		}
        if (shouldOpenNewInstance) {
            ImGui::OpenPopup("Add New Instance");
            shouldOpenNewInstance = false;
        }

        ImGui::PopStyleVar(2);

        ImGuiID dockspace_id = ImGui::GetID("LunaticDockspace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoWindowMenuButton);

        ImGui::End();

        static bool dockInitialized = false;
        if (!dockInitialized) {
            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_None);
            ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);

            ImGuiID dock_main_id = dockspace_id;
            ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.20f, nullptr, &dock_main_id);
            ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);
            ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.25f, nullptr, &dock_main_id);
            ImGuiID dock_id_center = dock_main_id;

            ImGui::DockBuilderDockWindow("Hierarchy", dock_id_left);
            ImGui::DockBuilderDockWindow("Inspector", dock_id_right);
            ImGui::DockBuilderDockWindow("Viewport", dock_id_center);
            ImGui::DockBuilderDockWindow("Console", dock_id_bottom);

            ImGui::DockBuilderFinish(dockspace_id);
            dockInitialized = true;
        }

		// If we are in play mode, update the engine
        if (isPlaying) {
            for (auto& updateable : engine.getUpdateables()) {
                updateable->update();
            }
			Lunatic::PhysicsSprite::stepAll(1.0f / 60.0f);
		}

        if (showViewportWindow) {
            static auto viewportSize = ImVec2(800, 600);
            ImGui::Begin("Viewport");
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            ImVec2 vpSize = ImGui::GetContentRegionAvail();
            int vpW = std::max(1, (int)vpSize.x);
            int vpH = std::max(1, (int)vpSize.y);
            framebuffer.resize(vpW, vpH);
            ImVec2 vpPos = ImGui::GetWindowPos();
            vpPos += ImGui::GetStyle().WindowPadding;
            vpPos.y += ImGui::GetFrameHeight();
            renderer.setViewport(static_cast<int>(vpPos.x), static_cast<int>(vpPos.y), vpW, vpH);
            framebuffer.bind();
            renderer.render(engine.getRenderables());
            framebuffer.unbind();
            glViewport(0, 0, display_w, display_h);
            ImGui::Image((ImTextureID)(uintptr_t)framebuffer.texture, vpSize, ImVec2(0, 1), ImVec2(1, 0));
            ImGui::End();
        }

        if (showHierarchyWindow) {
            ImGui::Begin("Hierarchy");
            std::function<void(std::shared_ptr<Lunatic::Instance>)> drawInstanceNode;
            drawInstanceNode = [&](std::shared_ptr<Lunatic::Instance> instance) {
                ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
                if (instance == selectedInstance.lock()) {
                    node_flags |= ImGuiTreeNodeFlags_Selected;
                }
                if (instance->getChildren().empty()) {
                    node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                }
                bool node_open = ImGui::TreeNodeEx((void*)(uintptr_t)instance.get(), node_flags, "%s", instance->getName().data());
                if (ImGui::IsItemClicked()) {
                    selectedInstance = instance;
                }
                if (node_open && !(node_flags & ImGuiTreeNodeFlags_NoTreePushOnOpen)) {
                    for (const auto& child : instance->getChildren()) {
                        drawInstanceNode(child);
                    }
                    ImGui::TreePop();
                }
                };
            //drawInstanceNode(engine.rootInstance);
            for (auto& child : engine.rootInstance->getChildren())
                drawInstanceNode(child);
            ImGui::End();
        }

        if (showInspectorWindow) {
            ImGui::Begin("Inspector");
            if (auto sel = selectedInstance.lock()) {
                struct TextBuf { std::array<char, 512> buf{}; std::string lastPropValue; bool initialized = false; };
                static void* lastSelPtr = nullptr;
                static std::unordered_map<std::string, TextBuf> textBuffers;

                auto& ti = sel->typeInfo;
                auto props = ti.get_properties();

                if (lastSelPtr != sel.get()) { lastSelPtr = sel.get(); textBuffers.clear(); }

                std::vector<rttr::property> vectorProps;
                for (const auto& prop : props) {
                    auto name = prop.get_name().to_string();
                    auto type = prop.get_type();
                    rttr::variant obj = std::ref(*sel);
                    auto value = prop.get_value(obj);
                    if (!value) continue;

                    // if the prop name is `children` skip it, we don't want to show that
                    if (name == "children") continue;

                    if (type == rttr::type::get<std::vector<std::shared_ptr<Lunatic::Instance>>>() || type.is_sequential_container()) {
                        vectorProps.push_back(prop);
                        continue;
                    }

                    ImGui::PushID(name.c_str());
                    ImGui::BeginDisabled(prop.is_readonly());

                    if (type.is_arithmetic()) {
                        if (type == rttr::type::get<int>()) {
                            int v = value.to_int();
                            if (ImGui::DragInt(name.c_str(), &v, 1.0f)) prop.set_value(obj, v);
                        }
                        else if (type == rttr::type::get<float>()) {
                            float v = value.to_float();
                            if (ImGui::DragFloat(name.c_str(), &v, 0.1f)) prop.set_value(obj, v);
                        }
                        else if (type == rttr::type::get<double>()) {
                            double v = value.to_double();
                            if (ImGui::DragScalar(name.c_str(), ImGuiDataType_Double, &v, 0.1f)) prop.set_value(obj, v);
                        }
                        else if (type == rttr::type::get<bool>()) {
                            bool v = value.to_bool();
                            if (ImGui::Checkbox(name.c_str(), &v)) prop.set_value(obj, v);
                        }
                        else {
                            ImGui::Text("%s: <unhandled arithmetic>", name.c_str());
                        }
                    }
                    else if (type.is_class()) {
                        if (type == rttr::type::get<std::string>() || type == rttr::type::get<std::string_view>()) {
                            auto& tb = textBuffers[name];
                            if (!tb.initialized) {
                                if (type == rttr::type::get<std::string>()) {
                                    tb.lastPropValue = value.get_value<std::string>();
                                }
                                else {
                                    tb.lastPropValue = std::string(value.get_value<std::string_view>());
                                }
                                memset(tb.buf.data(), 0, tb.buf.size());
                                strncpy_s(tb.buf.data(), tb.buf.size(), tb.lastPropValue.c_str(), _TRUNCATE);
                                tb.initialized = true;
                            }

                            bool submitted = ImGui::InputText(name.c_str(), tb.buf.data(), (int)tb.buf.size(), ImGuiInputTextFlags_EnterReturnsTrue);
                            bool commit = submitted || ImGui::IsItemDeactivatedAfterEdit();
                            if (commit) {
                                const char* text = tb.buf.data();
                                if (type == rttr::type::get<std::string>()) {
                                    prop.set_value(obj, std::string(text));
                                }
                                else {
                                    prop.set_value(obj, std::string_view(text));
                                }
                                tb.lastPropValue = text;
                            }
                        }
                        else if (type == rttr::type::get<glm::vec2>()) {
                            glm::vec2 v = value.get_value<glm::vec2>();
                            if (ImGui::DragFloat2(name.c_str(), &v.x, 0.1f)) prop.set_value(obj, v);
                        }
                        else if (type == rttr::type::get<glm::vec3>()) {
                            glm::vec3 v = value.get_value<glm::vec3>();
                            if (ImGui::DragFloat3(name.c_str(), &v.x, 0.1f)) prop.set_value(obj, v);
                        }
                        else if (type == rttr::type::get<glm::vec4>()) {
                            glm::vec4 v = value.get_value<glm::vec4>();
                            if (ImGui::DragFloat4(name.c_str(), &v.x, 0.1f)) prop.set_value(obj, v);
                        }
                        else if (type == rttr::type::get<std::shared_ptr<Lunatic::Instance>>()) {
                        }
                        else {
                            ImGui::Text("%s: <unhandled class %s>", name.c_str(), type.get_name().to_string().c_str());
                        }
                    }
                    else if (type.is_pointer()) {
                    }
                    else {
                        ImGui::Text("%s: <unhandled type %s>", name.c_str(), type.get_name().to_string().c_str());
                    }

                    ImGui::EndDisabled();
                    ImGui::PopID();
                }

                for (const auto& prop : vectorProps) {
                    auto name = prop.get_name().to_string();
                    auto type = prop.get_type();
                    rttr::variant obj = std::ref(*sel);
                    auto value = prop.get_value(obj);
                    if (!value) continue;

                    ImGui::PushID((std::string("vec_") + name).c_str());

                    if (value.get_type().is_sequential_container()) {
                        rttr::variant_sequential_view view = value.create_sequential_view();
                        if (view.get_size() == 0) {
                            ImGui::PopID();
                            continue;
                        }
                    }

                    if (ImGui::CollapsingHeader(name.data())) {
                        ImGui::Indent();

                        if (type == rttr::type::get<std::vector<std::shared_ptr<Lunatic::Instance>>>()) {
                            auto& vec = value.get_value<std::vector<std::shared_ptr<Lunatic::Instance>>>();
                            for (size_t i = 0; i < vec.size(); ++i) {
                                auto& inst = vec[i];
                                std::string label = inst ? std::string(inst->getName()) : std::string("<null>");
                                bool isSel = inst && (inst == selectedInstance.lock());
                                if (inst) {
                                    if (ImGui::Selectable((label + "##" + std::to_string(i)).c_str(), isSel)) {
                                        selectedInstance = inst;
                                    }
                                }
                                else {
                                    ImGui::Selectable((label + "##" + std::to_string(i)).c_str(), false);
                                }
                            }
                        }
                        else {
                            ImGui::BeginDisabled(true);
                            ImGui::Text("%s: <vector>", name.c_str());
                            ImGui::EndDisabled();
                        }

                        ImGui::Unindent();
                    }
                    ImGui::PopID();
                }
            }
            else {
                ImGui::Text("Select an instance to see properties.");
            }
            ImGui::End();
        }

        if (showConsoleWindow) {
            ImGui::Begin("Console");
            ImGui::TextDisabled("No console output yet.");
            ImGui::End();
        }
    });

    return 0;
}