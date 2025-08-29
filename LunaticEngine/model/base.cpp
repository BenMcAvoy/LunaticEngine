#include "pch.h"

#include "base.h"

#include "core/engine.h"

#include "render/renderer.h"

using namespace Lunatic;

Instance::Instance(std::string_view name) : name_(name), typeInfo(rttr::type::get<Instance>()) {
}

std::string_view Instance::getName() const {
	return name_;
}

void Instance::setName(std::string_view name) {
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

std::shared_ptr<Instance> Instance::findChildByName(std::string_view name) const {
	for (const auto& child : children_) {
		if (child->getName() == name) {
			return child;
		}
	}
	return nullptr;
}

std::string_view Instance::getClassName() const {
	return "Instance";
}

nlohmann::json Instance::serialize() const {
	nlohmann::json j;
	j["className"] = getClassName();
	j["name"] = name_;

	// everything else is done via properties (reflectable members)
	auto props = typeInfo.get_properties();
	for (const auto& prop : props) {
		auto propName = prop.get_name().to_string();
		rttr::variant obj = std::ref(const_cast<Instance&>(*this));
		auto propValue = prop.get_value(obj);
		if (propValue.is_valid()) {
			// Handle basic types; for complex types, you might need to implement custom serialization
			if (propValue.is_type<int>()) {
				j[propName] = propValue.get_value<int>();
			}
			else if (propValue.is_type<float>()) {
				j[propName] = propValue.get_value<float>();
			}
			else if (propValue.is_type<bool>()) {
				j[propName] = propValue.get_value<bool>();
			}
			else if (propValue.is_type<std::string>()) {
				auto& str = propValue.get_value<std::string>();
				if (!str.empty()) j[propName] = str;
			}
			else if (propValue.is_type<std::string_view>()) {
				auto& strView = propValue.get_value<std::string_view>();
				if (!strView.empty()) j[propName] = std::string(strView);
			}
			else if (propValue.is_type<std::filesystem::path>()) {
				auto& path = propValue.get_value<std::filesystem::path>();
				if (!path.empty()) j[propName] = path.string();
			}
			else if (propValue.is_type<glm::vec3>()) {
				auto& vec = propValue.get_value<glm::vec3>();
				j[propName] = { vec.x, vec.y, vec.z };
			}
			else if (propValue.is_type<glm::vec2>()) {
				auto& vec = propValue.get_value<glm::vec2>();
				j[propName] = { vec.x, vec.y };
			}
			else if (propValue.is_type<glm::vec4>()) {
				auto& vec = propValue.get_value<glm::vec4>();
				j[propName] = { vec.x, vec.y, vec.z, vec.w };
			}
			else {
				spdlog::trace("Skipping unsupported property type for serialization: {} ({})", propName, propValue.get_type().get_name().to_string());
			}
		}
	}

	// Serialize children
	if (!children_.empty()) {
		j["children"] = nlohmann::json::array();
		for (const auto& child : children_) {
			j["children"].push_back(child->serialize());
		}
	}

	return j;
}

void Instance::deserialize(const nlohmann::json& j) {
	if (parent_.lock() == nullptr) {
		if (j.contains("name") && j["name"].is_string()) {
			name_ = j["name"].get<std::string>();
			spdlog::info("Deserializing Instance from JSON for '{}'", name_);
		}
	}

	auto properties = typeInfo.get_properties();
	for (const auto& prop : properties) {
		auto propName = prop.get_name().to_string();
		if (j.contains(propName)) {
			rttr::variant obj = std::ref(*this);
			auto propValue = prop.get_value(obj);
			if (propValue.is_valid()) {
				auto& jsonValue = j[propName];

				if (propValue.is_type<int>() && jsonValue.is_number_integer()) {
					prop.set_value(obj, jsonValue.get<int>());
				}
				else if (propValue.is_type<float>() && jsonValue.is_number_float()) {
					prop.set_value(obj, jsonValue.get<float>());
				}
				else if (propValue.is_type<bool>() && jsonValue.is_boolean()) {
					prop.set_value(obj, jsonValue.get<bool>());
				}
				else if (propValue.is_type<std::string>() && jsonValue.is_string()) {
					prop.set_value(obj, jsonValue.get<std::string>());
				}
				else if (propValue.is_type<std::string_view>() && jsonValue.is_string()) {
					prop.set_value(obj, jsonValue.get<std::string_view>());
				}
				else if (propValue.is_type<std::filesystem::path>() && jsonValue.is_string()) {
					prop.set_value(obj, std::filesystem::path(jsonValue.get<std::string>()));
				}
				else if (propValue.is_type<glm::vec3>() && jsonValue.is_array() && jsonValue.size() == 3) {
					glm::vec3 vec;
					vec.x = jsonValue[0].get<float>();
					vec.y = jsonValue[1].get<float>();
					vec.z = jsonValue[2].get<float>();
					prop.set_value(obj, vec);
				}
				else if (propValue.is_type<glm::vec2>() && jsonValue.is_array() && jsonValue.size() == 2) {
					glm::vec2 vec;
					vec.x = jsonValue[0].get<float>();
					vec.y = jsonValue[1].get<float>();
					prop.set_value(obj, vec);
				}
				else if (propValue.is_type<glm::vec4>() && jsonValue.is_array() && jsonValue.size() == 4) {
					glm::vec4 vec;
					vec.x = jsonValue[0].get<float>();
					vec.y = jsonValue[1].get<float>();
					vec.z = jsonValue[2].get<float>();
					vec.w = jsonValue[3].get<float>();
					prop.set_value(obj, vec);
				}
				else {
					spdlog::trace("Skipping unsupported property type for deserialization: {} ({})", propName, propValue.get_type().get_name().to_string());
				}
			}
			else {
				spdlog::warn("Property '{}' exists but is not valid on object of type '{}'", propName, typeInfo.get_name().to_string());
			}
		}
		else {
#ifdef _DEBUG
			spdlog::trace("JSON does not contain property '{}', skipping", propName);
#endif
		}
	}

	// We most construct and restore children
	// Children should be constructed via RTTR so that they have the correct type
	if (j.contains("children") && j["children"].is_array()) {
		for (const auto& childJson : j["children"]) {
			if (childJson.contains("className") && childJson["className"].is_string()) {
				std::string className = childJson["className"].get<std::string>();
				rttr::type childType = rttr::type::get_by_name(className);
				if (childType.is_valid()) {
					std::string_view childName = "Unnamed";
					if (childJson.contains("name") && childJson["name"].is_string()) {
						childName = childJson["name"].get<std::string>();
					}

					// call `construct` method to create a std::shared_ptr of the correct type
					rttr::method constructMethod = childType.get_method("construct");
					if (constructMethod.is_valid()) {
						rttr::variant arg = childName;
						rttr::variant var = constructMethod.invoke({}, arg);
						if (var.is_type<std::shared_ptr<Instance>>()) {
							auto& childInstance = var.get_value<std::shared_ptr<Instance>>();
							if (childInstance) {
								childInstance->setName(childName);
								childInstance->setParent(shared_from_this());
								childInstance->deserialize(childJson);
							}
						}
						else {
							spdlog::warn("Construct method for class '{}' did not return std::shared_ptr<Instance> but instead returned '{}'", className, var.get_type().get_name().to_string());
						}
					}
					else {
						spdlog::warn("No construct method found for class '{}' during deserialization", className);
					}
				}
				else {
					spdlog::warn("Unknown or invalid class name '{}' for child during deserialization", className);
				}
			}
		}
	}
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