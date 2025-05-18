#include "pch.h"

#include "datamodel.h"

namespace Lunatic {
	// ---------------------------
	// Instance Implementation
	// ---------------------------

	Instance::Instance(std::string_view name, std::string_view className)
		: name(name), className(className) {}

	Instance::~Instance() {
		// Clear children so shared_ptrs are released properly
		children.clear();
	}

	void Instance::setParent(std::shared_ptr<Instance> newParent) {
		if (auto oldParent = parent.lock()) {
			oldParent->removeChild(shared_from_this());
		}

		parent = newParent;

		if (newParent) {
			newParent->addChild(shared_from_this());
		}
	}

	std::shared_ptr<Instance> Instance::getParent() const {
		return parent.lock();
	}

	void Instance::addChild(std::shared_ptr<Instance> child) {
		// Avoid duplicates
		if (std::find(children.begin(), children.end(), child) != children.end())
			return;

		children.push_back(child);
		child->parent = shared_from_this();
	}

	void Instance::removeChild(std::shared_ptr<Instance> child) {
		auto it = std::remove(children.begin(), children.end(), child);
		if (it != children.end()) {
			children.erase(it, children.end());
			child->parent.reset();
		}
	}

	std::shared_ptr<Instance> Instance::find(std::string_view name) {
		for (auto& child : children) {
			if (child->getName() == name)
				return child;
		}
		return nullptr;
	}

	std::vector<std::shared_ptr<Instance>> Instance::findAll(std::string_view name) {
		std::vector<std::shared_ptr<Instance>> matches;
		for (auto& child : children) {
			if (child->getName() == name)
				matches.push_back(child);
		}
		return matches;
	}

	const std::string& Instance::getName() const {
		return name;
	}

	void Instance::setName(std::string_view newName) {
		name = newName;
	}

	const std::string& Instance::getClassName() const {
		return className;
	}

	// ---------------------------
	// InstanceRegistry
	// ---------------------------

	static std::unordered_map<std::string, InstanceRegistry::Factory>& GetRegistry() {
		static std::unordered_map<std::string, InstanceRegistry::Factory> registry;
		return registry;
	}

	void InstanceRegistry::Register(std::string_view name, Factory factory) {
		GetRegistry()[std::string(name)] = std::move(factory);
	}

	std::shared_ptr<Instance> InstanceRegistry::Create(std::string_view name) {
		auto it = GetRegistry().find(std::string(name));
		if (it != GetRegistry().end()) {
			return it->second();
		}
		throw std::runtime_error("InstanceRegistry: Unknown type: " + std::string(name));
	}

	// ---------------------------
	// Sprite Implementation
	// ---------------------------

	Sprite::Sprite()
		: RenderableInstance("Sprite", "Sprite") {}

	void Sprite::setPosition(const glm::vec2& pos) {
		position = pos;
	}

	const glm::vec2& Sprite::getPosition() const {
		return position;
	}

	void Sprite::setTexture(std::string_view path) {
		texturePath = path;
	}

	void Sprite::setColour(const glm::vec3& col) {
		colour = col;
	}

	// ---------------------------
	// Workspace Implementation
	// ---------------------------

	void Workspace::onUpdate(float deltaTime) {
		spdlog::trace("Workspace::onUpdate");

		for (auto& child : children) {
			if (auto service = std::dynamic_pointer_cast<Service>(child)) {
				service->onUpdate(deltaTime); // child services
			}
		}
	}

	void Workspace::onRender(const RenderContext& context) {
		spdlog::trace("Workspace::onRender");

		for (auto& child : children) {
			if (auto service = std::dynamic_pointer_cast<RenderableService>(child)) {
				service->onRender(context); // child renderable services
			}
		}
	}

	// ---------------------------
	// LuaService Implementation
	// ---------------------------

	void LuaManager::onUpdate(float deltaTime) {
		spdlog::trace("LuaService::onUpdate");

		auto workspace = ServiceLocator::Get<Workspace>("Workspace");
		spdlog::trace("Workspace name: {}", workspace->getName());
	}

	// ---------------------------
	// PhysicsService Implementation
	// ---------------------------

	void Physics::onUpdate(float deltaTime) {
		spdlog::trace("PhysicsService::onUpdate");
	}
} // namespace Lunatic


/*#include "scene.h"

using namespace Lunatic;

constexpr float vertexData[] = {
	-0.5f, -0.5f, 0.0f, 0.0f,
	0.5f, -0.5f, 1.0f, 0.0f,
	0.5f,  0.5f, 1.0f, 1.0f,
   -0.5f,  0.5f, 0.0f, 1.0f
};

constexpr std::uint32_t indexData[] = { 0, 1, 2, 2, 3, 0 };

Scene::Scene() {
    rectBuffers.uploadData(
        std::span<const float>(vertexData, std::size(vertexData)),
        std::span<const std::uint32_t>(indexData, std::size(indexData))
    );
    rectBuffers.setAttribute(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)0);
    rectBuffers.setAttribute(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(sizeof(float) * 2));
}

void Scene::addObject(const std::shared_ptr<Object>& object) {
    objects.push_back(object);
    object->parent.reset();
}

void Scene::removeObject(const std::shared_ptr<Object>& obj) {
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
}*/
