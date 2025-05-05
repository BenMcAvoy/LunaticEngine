#include "pch.h"

#include "lunatic/scene.h"

constexpr float vertexData[] = {
	-0.5f, -0.5f, 0.0f, 0.0f,
	0.5f, -0.5f, 1.0f, 0.0f,
	0.5f,  0.5f, 1.0f, 1.0f,
   -0.5f,  0.5f, 0.0f, 1.0f
};

constexpr std::uint32_t indexData[] = { 0, 1, 2, 2, 3, 0 };

Lunatic::Scene::Scene() {
    rectBuffers.uploadData(
        std::span<const float>(vertexData, std::size(vertexData)),
        std::span<const std::uint32_t>(indexData, std::size(indexData))
    );
    rectBuffers.setAttribute(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)0);
    rectBuffers.setAttribute(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(sizeof(float) * 2));
}

void Lunatic::Scene::addObject(const std::shared_ptr<Object>& object) {
    objects.push_back(object);
    object->parent.reset();
}

void Lunatic::Scene::removeObject(const std::shared_ptr<Object>& obj) {
    std::vector<std::shared_ptr<Object>> stack{ obj };
    std::unordered_set<std::shared_ptr<Object>> toRemove;
    while (!stack.empty()) {
        auto current = stack.back();
        stack.pop_back();
        if (!toRemove.insert(current).second)
            continue;  // already visited
        for (auto& child : current->children) {
            stack.push_back(child);
        }
    }

    if (auto p = obj->parent.lock()) {
        auto& siblings = p->children;
        siblings.erase(
            std::remove_if(siblings.begin(), siblings.end(),
                [&](auto& sp) { return sp == obj; }),
            siblings.end()
        );
    }

    for (auto& rem : toRemove) {
        rem->children.clear();
        rem->parent.reset();
    }

    objects.erase(
        std::remove_if(objects.begin(), objects.end(),
            [&](auto& o) { return toRemove.count(o) != 0; }),
        objects.end()
    );
}
