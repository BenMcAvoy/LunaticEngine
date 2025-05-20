#include "pch.h"

#include "base.h"

namespace Lunatic {

	// ----- Instance ----- //

	Instance::Instance(std::string_view name, std::string_view className)
		: name(name), className(className) {}

	Instance::~Instance() = default;

	void Instance::setParent(std::shared_ptr<Instance> newParent) {
		if (auto currentParent = parent.lock()) {
			auto& siblings = currentParent->children;
			siblings.erase(std::remove(siblings.begin(), siblings.end(), shared_from_this()), siblings.end());
		}

		parent = newParent;
		if (newParent) {
			newParent->children.push_back(shared_from_this());
		}
	}

	std::shared_ptr<Instance> Instance::getParent() const {
		return parent.lock();
	}

	void Instance::addChild(std::shared_ptr<Instance> child) {
		child->setParent(shared_from_this());
	}

	void Instance::removeChild(std::shared_ptr<Instance> child) {
		children.erase(std::remove(children.begin(), children.end(), child), children.end());
		child->parent.reset();
	}

	std::shared_ptr<Instance> Instance::find(std::string_view name) {
		for (auto& child : children) {
			if (child->name == name) {
				return child;
			}
		}
		return nullptr;
	}

	std::vector<std::shared_ptr<Instance>> Instance::findAll(std::string_view name) {
		std::vector<std::shared_ptr<Instance>> matches;
		for (auto& child : children) {
			if (child->name == name) {
				matches.push_back(child);
			}
		}
		return matches;
	}

	std::string_view Instance::getName() {
		return name;
	}

	void Instance::setName(std::string_view newName) {
		name = newName;
	}

	std::string_view Instance::getClassName() {
		return className;
	}

	// ----- InstanceRegistry ----- //

	static std::unordered_map<std::string, InstanceRegistry::Factory>& getRegistry() {
		static std::unordered_map<std::string, InstanceRegistry::Factory> registry;
		return registry;
	}

	void InstanceRegistry::Register(std::string_view name, Factory factory) {
		getRegistry()[std::string(name)] = std::move(factory);
	}

	std::shared_ptr<Instance> InstanceRegistry::Create(std::string_view name) {
		auto it = getRegistry().find(std::string(name));
		if (it != getRegistry().end()) {
			return it->second();
		}
		return nullptr;
	}

	// ----- Service ----- //

	Service::Service(std::string_view name)
		: Instance(name, "Service") {}

} // namespace Lunatic
