#pragma once

#include "pch.h"

#include "lunatic/shader.h"
#include "lunatic/buffers.h"
#include "lunatic/camera.h"

namespace Lunatic {
    struct Object : public std::enable_shared_from_this<Object> {
        // Hierarchy
        std::weak_ptr<Object> parent;
        std::vector<std::shared_ptr<Object>> children;

        // Datare 
		char name[64] = "Object";
		glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f }; // RGBA

        // Transform
		glm::vec3 position = { 0.0f, 0.0f, 0.0f };
        glm::vec2 scale = { 1.0f, 1.0f };
        float rotation = 0.0f;

        void attachChild(const std::shared_ptr<Object>& child) {
            if (auto oldP = child->parent.lock()) {
                oldP->detachChild(child);
            }
            child->parent = shared_from_this();
            children.push_back(child);
        }

        void detachChild(const std::shared_ptr<Object>& child) {
            children.erase(
                std::remove_if(
                    children.begin(), children.end(),
                    [&](auto& c) { return c == child; }
                ),
                children.end()
            );
            if (child->parent.lock() == shared_from_this())
                child->parent.reset();
        }
    };

    class Scene {
    public:
        Scene();
		~Scene() = default;

    public:
        std::vector<std::shared_ptr<Object>> objects;

        void addObject(const std::shared_ptr<Object>& object);
        void removeObject(const std::shared_ptr<Object>& obj);

		Shader shader;
        Buffers rectBuffers;
		Camera camera;
    };
} // namespace Lunatic
