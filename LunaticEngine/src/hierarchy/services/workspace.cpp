#include "pch.h"

#include "workspace.h"
#include "../objects/sprite.h"

using namespace Lunatic::Services;

Workspace::Workspace() : Service("Workspace") {
	// Constructor does not set up hierarchy to avoid std::bad_weak_ptr
}

void Workspace::initialize() {
	// Create some example entities and show some hierarchy stuff ig

	auto sprite1 = std::make_shared<Sprite>("Sprite1");
	auto sprite2 = std::make_shared<Sprite>("Sprite2");
	auto sprite3 = std::make_shared<Sprite>("Sprite3");

	// Make sprite2 a child of sprite1
	sprite1->addChild(sprite2);

	// Add sprite1 and sprite3 to the workspace (root)
	this->addChild(sprite1);
	this->addChild(sprite3);
}

void Workspace::update(float deltaTime) {
}

void Workspace::render() {
    // Render a Dear ImGui window showing the workspace hierarchy as a tree
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
                
                // Show position with editable fields
                ImGui::Text("Position:");
                ImGui::SameLine();
                float pos[2] = { selectedInstance->position.x, selectedInstance->position.y };
                if (ImGui::DragFloat2("##Position", pos, 0.1f)) {
                    selectedInstance->position.x = pos[0];
                    selectedInstance->position.y = pos[1];
                }
                
                // Show parent info
                auto parent = selectedInstance->getParent();
                if (parent) {
                    ImGui::Text("Parent: %s", parent->getName().data());
                } else {
                    ImGui::Text("Parent: None (Root)");
                }
                
                // Show children count
                ImGui::Text("Children: %zu", selectedInstance->children.size());
                
                // List children
                if (!selectedInstance->children.empty()) {
                    ImGui::Text("Child List:");
                    ImGui::Indent();
                    for (const auto& child : selectedInstance->children) {
                        ImGui::BulletText("%s (%s)", child->getName().data(), child->getClassName().data());
                    }
                    ImGui::Unindent();
                }
                
                // Show if it's renderable
                auto renderable = std::dynamic_pointer_cast<IRenderable>(selectedInstance);
                ImGui::Text("Renderable: %s", renderable ? "Yes" : "No");
                
            } else {
                ImGui::Text("Select an instance to view properties");
            }
            
            ImGui::EndTable();
        }
    }
    ImGui::End();
}