#include "pch.h"

#include "workspace.h"
#include "hierarchy/objects/cube.h"

using namespace Lunatic::Services;

Workspace::Workspace() : Service("Workspace") {
	// Constructor does not set up hierarchy to avoid std::bad_weak_ptr
}

void Workspace::initialize() {
	// Create some example entities and show some hierarchy stuff ig

	auto cube1 = std::make_shared<Cube>("Cube1");
	auto cube2 = std::make_shared<Cube>("Cube2");
	auto cube3 = std::make_shared<Cube>("Cube3");

	// Set different 3D positions to test depth
	cube1->position = glm::vec3(-2.0f, 0.0f, 0.0f);
	cube2->position = glm::vec3(0.0f, 0.0f, -2.0f);  // Behind cube1
	cube3->position = glm::vec3(2.0f, 1.0f, -1.0f);  // To the right and slightly back

	// Make cube2 a child of cube1
	cube1->addChild(cube2);

	// Add cube1 and cube3 to the workspace (root)
	this->addChild(cube1);
	this->addChild(cube3);
}

void Workspace::update(float deltaTime) {
}

void Workspace::render() {
    // Workspace only handles UI rendering - the actual 3D scene rendering 
    // is handled by the Renderer service to avoid conflicts
    
    // Render the Dear ImGui workspace explorer window
    if (ImGui::Begin("Workspace Explorer")) {
        // Split window into two columns: tree view and properties
        if (ImGui::BeginTable("WorkspaceLayout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {
            ImGui::TableSetupColumn("Hierarchy", ImGuiTableColumnFlags_WidthFixed, 300.0f);
            ImGui::TableSetupColumn("Properties", ImGuiTableColumnFlags_WidthStretch);
            
            ImGui::TableNextRow();
            
            // Left column: Hierarchy tree
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Hierarchy");
            ImGui::Separator();
            
            static std::shared_ptr<Instance> selectedInstance = nullptr;
            
            std::function<void(const std::shared_ptr<Instance>&)> renderInstanceTree;
            renderInstanceTree = [&](const std::shared_ptr<Instance>& instance) {
                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
                if (instance->children.empty()) {
                    flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                }
                if (selectedInstance == instance) {
                    flags |= ImGuiTreeNodeFlags_Selected;
                }
                
                bool isOpen = ImGui::TreeNodeEx(instance->getName().data(), flags);
                
                // Handle selection
                if (ImGui::IsItemClicked()) {
                    selectedInstance = instance;
                }
                
                if (isOpen && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen)) {
                    for (const auto& child : instance->children) {
                        renderInstanceTree(child);
                    }
                    ImGui::TreePop();
                }
            };
            
            for (const auto& child : this->children) {
                renderInstanceTree(child);
            }
            
            // Right column: Properties
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("Properties");
            ImGui::Separator();
            
            if (selectedInstance) {
                ImGui::Text("Name: %s", selectedInstance->getName().data());
                ImGui::Text("Class: %s", selectedInstance->getClassName().data());
                
                ImGui::Text("Position:");
                ImGui::SameLine();
				ImGui::DragFloat3("##Position", &selectedInstance->position.x, 0.1f, -100.0f, 100.0f, "%.1f");
                
                ImGui::Text("Rotation:");
                ImGui::SameLine();
				ImGui::DragFloat3("##Rotation", &selectedInstance->rotation.x, 0.1f, 0.0f, 360.0f, "%.1f");
                
                auto parent = selectedInstance->getParent();
                if (parent) {
                    ImGui::Text("Parent: %s", parent->getName().data());
                } else {
                    ImGui::Text("Parent: None (Root)");
                }
                
                ImGui::Text("Children: %zu", selectedInstance->children.size());
                
                if (!selectedInstance->children.empty()) {
                    ImGui::Text("Child List:");
                    ImGui::Indent();
                    for (const auto& child : selectedInstance->children) {
                        ImGui::BulletText("%s (%s)", child->getName().data(), child->getClassName().data());
                    }
                    ImGui::Unindent();
                }
            } else {
                ImGui::Text("Select an instance to view properties");
            }
            
            ImGui::EndTable();
        }
    }
    ImGui::End();
}