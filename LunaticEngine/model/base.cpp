#include "pch.h"

#include "base.h"

#include "core/engine.h"

#include "render/renderer.h"

using namespace Lunatic;

Instance::Instance(const std::string_view name) : name_(name) {
	metaType = entt::resolve<Instance>();
}

std::string_view Instance::getName() const {
	return name_;
}

void Instance::setName(const std::string_view name) {
	name_ = name;
}

std::shared_ptr<Instance> Instance::getParent() const {
	return parent_.lock();
}

void Instance::setParent(std::shared_ptr<Instance> parent) {
	if (auto oldParent = parent_.lock()) {
		auto& siblings = oldParent->children_;
		// Remove this instance from the old parent's children
		siblings.erase(std::remove(siblings.begin(), siblings.end(), shared_from_this()), siblings.end());
	}

	parent_ = parent;
	if (parent) {
		parent->children_.push_back(shared_from_this());
	}

	onAncestorChanged();
}

const std::vector<std::shared_ptr<Instance>>& Instance::getChildren() const {
	return children_;
}

std::shared_ptr<Instance> Instance::findChildByName(std::string name) const {
	for (const auto& child : children_) {
		if (child->getName() == name) {
			return child;
		}
	}
	return nullptr;
}

std::string Instance::getClassName() const {
	return "Instance";
}

Renderable::Renderable() {
	Engine::getInstance().registerRenderable(this);
}
Renderable::~Renderable() {
	Engine::getInstance().unregisterRenderable(this);
}

Updateable::Updateable() {
	Engine::getInstance().registerUpdateable(this);
}
Updateable::~Updateable() {
	Engine::getInstance().unregisterUpdateable(this);
}