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
		// if it's readonly, skip it (why save data we can't load back?)
		if (prop.is_readonly()) continue;

		auto propValue = prop.get_value(obj);
		if (propValue.is_valid()) {
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

void Instance::clearChildren() {
	auto currentChildren = std::move(children_);
	children_.clear();
	for (auto& child : currentChildren) {
		if (child) {
			child->destroy();
		}
	}
	
	// If this is the root, purge engine registries to guarantee a clean slate
	if (Engine::getInstance().rootInstance.get() == this) {
		auto& eng = Engine::getInstance();
		eng.getRenderables().clear();
		eng.getUpdateables().clear();
	}
}

void Instance::destroy() {
	auto currentChildren = std::move(children_);
	children_.clear();
	for (auto& child : currentChildren) {
		if (child) {
			child->destroy();
		}
	}

	if (auto parent = parent_.lock()) {
		auto& siblings = parent->children_;
		siblings.erase(std::remove(siblings.begin(), siblings.end(), shared_from_this()), siblings.end());
		parent_.reset();
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

// Lua wrapper for per-instance key-value storage backed by Instance::luaUserData
struct LuaDataStore {
	std::shared_ptr<Instance> instance;

	explicit LuaDataStore(std::shared_ptr<Instance> inst) : instance(std::move(inst)) {}
	~LuaDataStore() = default;

	static LuaDataStore& fromLua(lua_State* L) {
		LuaDataStore* ud = static_cast<LuaDataStore*>(lua_touserdata(L, 1));
		assert(ud != nullptr && "Expected LuaDataStore userdata");
		return *ud;
	}

	static int indexFN(lua_State* L) {
		LuaDataStore& ds = fromLua(L);
		if (!ds.instance) { lua_pushnil(L); return 1; }
		if (!lua_isstring(L, 2)) { lua_pushnil(L); return 1; }
		const char* key = lua_tostring(L, 2);
		ds.instance->luaUserData.get(L, key);
		return 1;
	}

	static int newIndexFN(lua_State* L) {
		LuaDataStore& ds = fromLua(L);
		if (!ds.instance) return 0;
		if (!lua_isstring(L, 2)) return luaL_error(L, "LuaDataStore keys must be strings");
		size_t len = 0;
		const char* key = lua_tolstring(L, 2, &len);
		ds.instance->luaUserData.set(L, std::string_view(key, len), 3);
		return 0;
	}

	static void createInLua(const std::shared_ptr<Instance>& inst, lua_State* L) {
		LuaDataStore* ud = static_cast<LuaDataStore*>(lua_newuserdata(L, sizeof(LuaDataStore)));
		new (ud) LuaDataStore {inst};

		if (luaL_newmetatable(L, "LuaDataStoreMeta")) {
			lua_pushcfunction(L, indexFN);
			lua_setfield(L, -2, "__index");
			lua_pushcfunction(L, newIndexFN);
			lua_setfield(L, -2, "__newindex");
			lua_pushcfunction(L, [](lua_State* L) -> int {
				LuaDataStore* ud = static_cast<LuaDataStore*>(lua_touserdata(L, 1));
				ud->~LuaDataStore();
				return 0;
			});
			lua_setfield(L, -2, "__gc");
		}
		lua_setmetatable(L, -2);
	}
};

void LuaUserData::set(lua_State* L, std::string_view key, int index) {
	// Remove existing entry if it exists
	auto it = entries_.find(std::string(key));
	if (it != entries_.end()) {
		luaL_unref(L, LUA_REGISTRYINDEX, it->second);
	}

	lua_pushvalue(L, index); // copy to top
	int ref = luaL_ref(L, LUA_REGISTRYINDEX); // pops value

	// Store the reference in the map
	entries_[std::string(key)] = ref;
}

void LuaUserData::get(lua_State* L, std::string_view key) const {
	auto it = entries_.find(std::string(key));
	if (it == entries_.end()) {
		lua_pushnil(L); // not found
		return;
	}

	int ref = it->second;
	lua_rawgeti(L, LUA_REGISTRYINDEX, ref); // pushes the value onto the stack
}

int LuaInstance::indexFN(lua_State* L) {
	LuaInstance& ud = fromLua(L);
	std::string key = luaL_checkstring(L, 2);

	// Provide access to the per-instance data store via 'data'
	if (key == "data") {
		LuaDataStore::createInLua(ud.instance, L);
		return 1;
	}

	// First check if it's in the LuaUserData
	ud.instance->luaUserData.get(L, key);
	if (!lua_isnil(L, -1)) {
		return 1; // found in LuaUserData
	}
	lua_pop(L, 1); // remove nil

	// Next check if it's a property via RTTR
	auto prop = ud.instance->typeInfo.get_property(key);
	if (prop.is_valid()) {
		rttr::variant obj = std::ref(*ud.instance);
		auto propValue = prop.get_value(obj);
		if (propValue.is_valid()) {
			if (propValue.is_type<int>()) {
				lua_pushinteger(L, propValue.get_value<int>());
			}
			else if (propValue.is_type<float>()) {
				lua_pushnumber(L, propValue.get_value<float>());
			}
			else if (propValue.is_type<bool>()) {
				lua_pushboolean(L, propValue.get_value<bool>());
			}
			else if (propValue.is_type<std::string>()) {
				auto& str = propValue.get_value<std::string>();
				lua_pushstring(L, str.c_str());
			}
			else if (propValue.is_type<std::string_view>()) {
				auto& strView = propValue.get_value<std::string_view>();
				lua_pushlstring(L, strView.data(), strView.size());
			}
			else if (propValue.is_type<std::filesystem::path>()) {
				auto& path = propValue.get_value<std::filesystem::path>();
				std::string pathStr = path.string();
				lua_pushlstring(L, pathStr.c_str(), pathStr.size());
			}
			else if (propValue.is_type<glm::vec2>()) {
				auto& v = propValue.get_value<glm::vec2>();
				lua_newtable(L);
				lua_pushnumber(L, v.x); lua_setfield(L, -2, "x");
				lua_pushnumber(L, v.y); lua_setfield(L, -2, "y");
				// numeric indices
				lua_pushnumber(L, v.x); lua_rawseti(L, -2, 1);
				lua_pushnumber(L, v.y); lua_rawseti(L, -2, 2);
				// attach vector metatable (no-op if not present)
				luaL_getmetatable(L, "LuaVec2Meta");
				lua_setmetatable(L, -2);
			}
			else if (propValue.is_type<glm::vec3>()) {
				auto& v = propValue.get_value<glm::vec3>();
				lua_newtable(L);
				lua_pushnumber(L, v.x); lua_setfield(L, -2, "x");
				lua_pushnumber(L, v.y); lua_setfield(L, -2, "y");
				lua_pushnumber(L, v.z); lua_setfield(L, -2, "z");
				// aliases
				lua_pushnumber(L, v.x); lua_setfield(L, -2, "r");
				lua_pushnumber(L, v.y); lua_setfield(L, -2, "g");
				lua_pushnumber(L, v.z); lua_setfield(L, -2, "b");
				// numeric indices
				lua_pushnumber(L, v.x); lua_rawseti(L, -2, 1);
				lua_pushnumber(L, v.y); lua_rawseti(L, -2, 2);
				lua_pushnumber(L, v.z); lua_rawseti(L, -2, 3);
				luaL_getmetatable(L, "LuaVec3Meta");
				lua_setmetatable(L, -2);
			}
			else if (propValue.is_type<glm::vec4>()) {
				auto& v = propValue.get_value<glm::vec4>();
				lua_newtable(L);
				lua_pushnumber(L, v.x); lua_setfield(L, -2, "x");
				lua_pushnumber(L, v.y); lua_setfield(L, -2, "y");
				lua_pushnumber(L, v.z); lua_setfield(L, -2, "z");
				lua_pushnumber(L, v.w); lua_setfield(L, -2, "w");
				// aliases
				lua_pushnumber(L, v.x); lua_setfield(L, -2, "r");
				lua_pushnumber(L, v.y); lua_setfield(L, -2, "g");
				lua_pushnumber(L, v.z); lua_setfield(L, -2, "b");
				lua_pushnumber(L, v.w); lua_setfield(L, -2, "a");
				// numeric indices
				lua_pushnumber(L, v.x); lua_rawseti(L, -2, 1);
				lua_pushnumber(L, v.y); lua_rawseti(L, -2, 2);
				lua_pushnumber(L, v.z); lua_rawseti(L, -2, 3);
				lua_pushnumber(L, v.w); lua_rawseti(L, -2, 4);
				luaL_getmetatable(L, "LuaVec4Meta");
				lua_setmetatable(L, -2);
			}
			else if (propValue.is_type<std::shared_ptr<Instance>>()) {
				auto& ptr = propValue.get_value<std::shared_ptr<Instance>>();
				if (ptr) {
					LuaInstance::createInLua(ptr, L);
				} else {
					lua_pushnil(L);
				}
			}
			else if (propValue.is_type<std::vector<std::shared_ptr<Instance>>>()) {
				auto& vec = propValue.get_value<std::vector<std::shared_ptr<Instance>>>();
				lua_newtable(L);
				int i = 1;
				for (const auto& child : vec) {
					if (child) {
						LuaInstance::createInLua(child, L);
					} else {
						lua_pushnil(L);
					}
					lua_rawseti(L, -2, i++);
				}
			}
			else {
				spdlog::warn("Unsupported property type for LuaInstance __index: {} ({})", key, propValue.get_type().get_name().to_string());
				lua_pushnil(L);
			}

			return 1;
		}
		else {
			spdlog::warn("Property '{}' exists but is not valid on object of type '{}'", key, ud.instance->typeInfo.get_name().to_string());
		}
	}

	// Look for methods
	auto method = ud.instance->typeInfo.get_method(key);
	if (method.is_valid()) {
		// Upvalue 1: method name (string)
		lua_pushlstring(L, key.c_str(), key.size());
		// Upvalue 2: the userdata itself to keep it alive and accessible
		lua_pushvalue(L, 1);
		lua_pushcclosure(L, [](lua_State* L) -> int {
			// Fetch method name from upvalue 1
			size_t mlen = 0;
			const char* mname = lua_tolstring(L, lua_upvalueindex(1), &mlen);
			if (!mname) return luaL_error(L, "Invalid method upvalue");
			std::string methodName(mname, mlen);

			// Fetch userdata from upvalue 2 and get instance
			LuaInstance* ud2 = static_cast<LuaInstance*>(lua_touserdata(L, lua_upvalueindex(2)));
			if (!ud2 || !ud2->instance) return luaL_error(L, "Invalid instance upvalue");

			rttr::method method2 = ud2->instance->typeInfo.get_method(methodName);
			if (!method2.is_valid()) return luaL_error(L, "Unknown method: %s", methodName.c_str());

			int numArgs = lua_gettop(L);
			std::vector<rttr::argument> args;
			args.reserve(numArgs);
			// If called with colon syntax (obj:method(...)), arg 1 is the userdata (self).
			int startIdx = 1;
			if (numArgs >= 1 && lua_isuserdata(L, 1)) {
				// Check if it's our LuaInstance userdata by comparing metatable
				bool isSelf = false;
				if (lua_getmetatable(L, 1)) {                 // push mt
					luaL_getmetatable(L, "LuaInstanceMeta");  // push expected mt
					isSelf = lua_rawequal(L, -1, -2) != 0;
					lua_pop(L, 2); // pop both mts
				}
				if (isSelf) startIdx = 2;
			}
			// Discover expected parameter types if available to aid conversion
			std::vector<rttr::type> paramTypes;
			for (const auto& pi : method2.get_parameter_infos()) {
				paramTypes.emplace_back(pi.get_type());
			}
			int pIndex = 0;
			for (int i = startIdx; i <= numArgs; ++i, ++pIndex) {
				rttr::type pType = (pIndex >= 0 && pIndex < static_cast<int>(paramTypes.size())) ? paramTypes[pIndex] : rttr::type::get<void>();
				if (lua_isnumber(L, i)) {
					double num = lua_tonumber(L, i);
					if (pType == rttr::type::get<int>()) args.emplace_back(static_cast<int>(num));
					else if (pType == rttr::type::get<float>()) args.emplace_back(static_cast<float>(num));
					else args.emplace_back(num);
				}
				else if (lua_isstring(L, i)) {
					size_t len;
					const char* str = lua_tolstring(L, i, &len);
					args.emplace_back(std::string_view(str, len));
				}
				else if (lua_isboolean(L, i)) {
					args.emplace_back(lua_toboolean(L, i) != 0);
				}
				else if (lua_istable(L, i)) {
					if (pType == rttr::type::get<glm::vec2>()) {
						glm::vec2 v{0,0};
						lua_getfield(L, i, "x"); bool hasx = !lua_isnil(L, -1); v.x = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
						lua_getfield(L, i, "y"); bool hasy = !lua_isnil(L, -1); v.y = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
						if (!hasx || !hasy) {
							lua_rawgeti(L, i, 1); v.x = static_cast<float>(luaL_optnumber(L, -1, v.x)); lua_pop(L,1);
							lua_rawgeti(L, i, 2); v.y = static_cast<float>(luaL_optnumber(L, -1, v.y)); lua_pop(L,1);
						}
						args.emplace_back(v);
					}
					else if (pType == rttr::type::get<glm::vec3>()) {
						glm::vec3 v{0,0,0};
						lua_getfield(L, i, "x"); bool hasx = !lua_isnil(L, -1); if (hasx) v.x = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
						lua_getfield(L, i, "r"); bool hasr = !lua_isnil(L, -1); if (!hasx && hasr) v.x = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
						lua_getfield(L, i, "y"); bool hasy = !lua_isnil(L, -1); if (hasy) v.y = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
						lua_getfield(L, i, "g"); bool hasg = !lua_isnil(L, -1); if (!hasy && hasg) v.y = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
						lua_getfield(L, i, "z"); bool hasz = !lua_isnil(L, -1); if (hasz) v.z = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
						lua_getfield(L, i, "b"); bool hasb = !lua_isnil(L, -1); if (!hasz && hasb) v.z = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
						if (!(hasx||hasr) || !(hasy||hasg) || !(hasz||hasb)) {
							lua_rawgeti(L, i, 1); v.x = static_cast<float>(luaL_optnumber(L, -1, v.x)); lua_pop(L,1);
							lua_rawgeti(L, i, 2); v.y = static_cast<float>(luaL_optnumber(L, -1, v.y)); lua_pop(L,1);
							lua_rawgeti(L, i, 3); v.z = static_cast<float>(luaL_optnumber(L, -1, v.z)); lua_pop(L,1);
						}
						args.emplace_back(v);
					}
					else if (pType == rttr::type::get<glm::vec4>()) {
						glm::vec4 v{0,0,0,0};
						lua_getfield(L, i, "x"); bool hasx = !lua_isnil(L, -1); if (hasx) v.x = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
						lua_getfield(L, i, "r"); bool hasr = !lua_isnil(L, -1); if (!hasx && hasr) v.x = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
						lua_getfield(L, i, "y"); bool hasy = !lua_isnil(L, -1); if (hasy) v.y = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
						lua_getfield(L, i, "g"); bool hasg = !lua_isnil(L, -1); if (!hasy && hasg) v.y = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
						lua_getfield(L, i, "z"); bool hasz = !lua_isnil(L, -1); if (hasz) v.z = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
						lua_getfield(L, i, "b"); bool hasb = !lua_isnil(L, -1); if (!hasz && hasb) v.z = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
						lua_getfield(L, i, "w"); bool hasw = !lua_isnil(L, -1); if (hasw) v.w = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
						lua_getfield(L, i, "a"); bool hasa = !lua_isnil(L, -1); if (!hasw && hasa) v.w = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
						if (!(hasx||hasr) || !(hasy||hasg) || !(hasz||hasb) || !(hasw||hasa)) {
							lua_rawgeti(L, i, 1); v.x = static_cast<float>(luaL_optnumber(L, -1, v.x)); lua_pop(L,1);
							lua_rawgeti(L, i, 2); v.y = static_cast<float>(luaL_optnumber(L, -1, v.y)); lua_pop(L,1);
							lua_rawgeti(L, i, 3); v.z = static_cast<float>(luaL_optnumber(L, -1, v.z)); lua_pop(L,1);
							lua_rawgeti(L, i, 4); v.w = static_cast<float>(luaL_optnumber(L, -1, v.w)); lua_pop(L,1);
						}
						args.emplace_back(v);
					}
					else {
						glm::vec2 v{0,0};
						lua_getfield(L, i, "x"); bool hasx = !lua_isnil(L, -1); v.x = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
						lua_getfield(L, i, "y"); bool hasy = !lua_isnil(L, -1); v.y = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
						if (hasx && hasy) { args.emplace_back(v); }
						else {
							auto typeName = luaL_typename(L, i);
							spdlog::warn("Unsupported table argument at index {}: {}", i, typeName);
							return luaL_error(L, "Unsupported table argument at index %d", i);
						}
					}
				}
				else if (lua_isuserdata(L, i)) {
					// Try to interpret as LuaInstance userdata
					bool isLI = false;
					if (lua_getmetatable(L, i)) {
						luaL_getmetatable(L, "LuaInstanceMeta");
						isLI = lua_rawequal(L, -1, -2) != 0;
						lua_pop(L, 2);
					}
					if (isLI) {
						LuaInstance* argUd = static_cast<LuaInstance*>(lua_touserdata(L, i));
						if (argUd && argUd->instance) {
							args.emplace_back(argUd->instance);
							continue;
						}
					}
					// unknown userdata; report error
					auto typeName = luaL_typename(L, i);
					spdlog::warn("Unsupported userdata argument at index {}: {}", i, typeName);
					return luaL_error(L, "Unsupported userdata argument at index %d", i);
				}
				else {
					// debug log what the type is
					auto typeName = luaL_typename(L, i);
					return luaL_error(L, "Unsupported argument type at index %d (type %s)", i, typeName);
				}
			}

			rttr::variant obj = std::ref(*ud2->instance);
			rttr::variant result = method2.invoke_variadic(obj, args);
			if (result.is_valid()) {
				if (result.is_type<void>()) {
					return 0; // no return value
				}
				else if (result.is_type<int>()) {
					lua_pushinteger(L, result.get_value<int>());
					return 1;
				}
				else if (result.is_type<float>()) {
					lua_pushnumber(L, result.get_value<float>());
					return 1;
				}
				else if (result.is_type<bool>()) {
					lua_pushboolean(L, result.get_value<bool>());
					return 1;
				}
				else if (result.is_type<std::string>()) {
					auto& str = result.get_value<std::string>();
					lua_pushstring(L, str.c_str());
					return 1;
				}
				else if (result.is_type<std::string_view>()) {
					auto& strView = result.get_value<std::string_view>();
					lua_pushlstring(L, strView.data(), strView.size());
					return 1;
				}
				else if (result.is_type<std::filesystem::path>()) {
					auto& path = result.get_value<std::filesystem::path>();
					std::string pathStr = path.string();
					lua_pushlstring(L, pathStr.c_str(), pathStr.size());
					return 1;
				}
				else if (result.is_type<glm::vec2>()) {
					auto& v = result.get_value<glm::vec2>();
					lua_newtable(L);
					lua_pushnumber(L, v.x); lua_setfield(L, -2, "x");
					lua_pushnumber(L, v.y); lua_setfield(L, -2, "y");
					return 1;
				}
				else if (result.is_type<glm::vec3>()) {
					auto& v = result.get_value<glm::vec3>();
					lua_newtable(L);
					lua_pushnumber(L, v.x); lua_setfield(L, -2, "x");
					lua_pushnumber(L, v.y); lua_setfield(L, -2, "y");
					lua_pushnumber(L, v.z); lua_setfield(L, -2, "z");
					return 1;
				}
				else if (result.is_type<glm::vec4>()) {
					auto& v = result.get_value<glm::vec4>();
					lua_newtable(L);
					lua_pushnumber(L, v.x); lua_setfield(L, -2, "x");
					lua_pushnumber(L, v.y); lua_setfield(L, -2, "y");
					lua_pushnumber(L, v.z); lua_setfield(L, -2, "z");
					lua_pushnumber(L, v.w); lua_setfield(L, -2, "w");
					return 1;
				}
				else if (result.is_type<std::shared_ptr<Instance>>()) {
					auto& ptr = result.get_value<std::shared_ptr<Instance>>();
					if (ptr) {
						LuaInstance::createInLua(ptr, L);
					} else {
						lua_pushnil(L);
					}
					return 1;
				}
			}
			// If we reach here, push nil (we don't support other return types yet)
			spdlog::warn("Method '{}' on object of type '{}' returned unsupported type '{}'", methodName, ud2->instance->typeInfo.get_name().to_string(), result.get_type().get_name().to_string());
			lua_pushnil(L);
			return 1;
		}, 2);
		return 1;
	}
	else {
		spdlog::warn("No method '{}' found on object of type '{}'", key, ud.instance->typeInfo.get_name().to_string());
	}

	lua_pushnil(L);
	return 1;
}

int LuaInstance::newIndexFN(lua_State* L) {
	LuaInstance& ud = fromLua(L);
	std::string key = luaL_checkstring(L, 2);

	// Prevent overwriting the data store itself
	if (key == "data") {
		return luaL_error(L, "'data' is read-only!");
	}

	// First check if it's a property via RTTR
	auto prop = ud.instance->typeInfo.get_property(key);
	if (prop.is_valid()) {
		if (prop.is_readonly()) {
			spdlog::warn("Attempt to set readonly property '{}' on object of type '{}'", key, ud.instance->typeInfo.get_name().to_string());
			return 0;
		}

		rttr::variant obj = std::ref(*ud.instance);
		auto propValue = prop.get_value(obj);
			if (propValue.is_valid()) {
			if (propValue.is_type<int>() && lua_isnumber(L, 3)) {
				prop.set_value(obj, static_cast<int>(lua_tointeger(L, 3)));
			}
			else if (propValue.is_type<float>() && lua_isnumber(L, 3)) {
				prop.set_value(obj, static_cast<float>(lua_tonumber(L, 3)));
			}
			else if (propValue.is_type<bool>() && lua_isboolean(L, 3)) {
				prop.set_value(obj, lua_toboolean(L, 3) != 0);
			}
			else if (propValue.is_type<std::string>() && lua_isstring(L, 3)) {
				size_t len;
				const char* str = lua_tolstring(L, 3, &len);
				prop.set_value(obj, std::string(str, len));
			}
			else if (propValue.is_type<std::string_view>() && lua_isstring(L, 3)) {
				size_t len;
				const char* str = lua_tolstring(L, 3, &len);
				// Store a copy in a std::string to ensure the data remains valid
				std::string strCopy(str, len);
				prop.set_value(obj, std::string_view(strCopy));
			}
			else if (propValue.is_type<std::filesystem::path>() && lua_isstring(L, 3)) {
				size_t len;
				const char* str = lua_tolstring(L, 3, &len);
				prop.set_value(obj, std::filesystem::path(std::string(str, len)));
			}
				else if (propValue.is_type<glm::vec2>() && lua_istable(L, 3)) {
					glm::vec2 v{0,0};
					lua_getfield(L, 3, "x"); bool hasx = !lua_isnil(L, -1); if (hasx) v.x = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
					lua_getfield(L, 3, "y"); bool hasy = !lua_isnil(L, -1); if (hasy) v.y = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
					if (!hasx || !hasy) { lua_rawgeti(L, 3, 1); v.x = static_cast<float>(luaL_optnumber(L, -1, v.x)); lua_pop(L,1); lua_rawgeti(L, 3, 2); v.y = static_cast<float>(luaL_optnumber(L, -1, v.y)); lua_pop(L,1);} 
					prop.set_value(obj, v);
				}
				else if (propValue.is_type<glm::vec3>() && lua_istable(L, 3)) {
					glm::vec3 v{0,0,0};
					lua_getfield(L, 3, "x"); bool hasx = !lua_isnil(L, -1); if (hasx) v.x = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
					lua_getfield(L, 3, "r"); bool hasr = !lua_isnil(L, -1); if (!hasx && hasr) v.x = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
					lua_getfield(L, 3, "y"); bool hasy = !lua_isnil(L, -1); if (hasy) v.y = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
					lua_getfield(L, 3, "g"); bool hasg = !lua_isnil(L, -1); if (!hasy && hasg) v.y = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
					lua_getfield(L, 3, "z"); bool hasz = !lua_isnil(L, -1); if (hasz) v.z = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
					lua_getfield(L, 3, "b"); bool hasb = !lua_isnil(L, -1); if (!hasz && hasb) v.z = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
					if (!(hasx||hasr) || !(hasy||hasg) || !(hasz||hasb)) { lua_rawgeti(L, 3, 1); v.x = static_cast<float>(luaL_optnumber(L, -1, v.x)); lua_pop(L,1); lua_rawgeti(L, 3, 2); v.y = static_cast<float>(luaL_optnumber(L, -1, v.y)); lua_pop(L,1); lua_rawgeti(L, 3, 3); v.z = static_cast<float>(luaL_optnumber(L, -1, v.z)); lua_pop(L,1);} 
					prop.set_value(obj, v);
				}
				else if (propValue.is_type<glm::vec4>() && lua_istable(L, 3)) {
					glm::vec4 v{0,0,0,0};
					lua_getfield(L, 3, "x"); bool hasx = !lua_isnil(L, -1); if (hasx) v.x = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
					lua_getfield(L, 3, "r"); bool hasr = !lua_isnil(L, -1); if (!hasx && hasr) v.x = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
					lua_getfield(L, 3, "y"); bool hasy = !lua_isnil(L, -1); if (hasy) v.y = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
					lua_getfield(L, 3, "g"); bool hasg = !lua_isnil(L, -1); if (!hasy && hasg) v.y = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
					lua_getfield(L, 3, "z"); bool hasz = !lua_isnil(L, -1); if (hasz) v.z = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
					lua_getfield(L, 3, "b"); bool hasb = !lua_isnil(L, -1); if (!hasz && hasb) v.z = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
					lua_getfield(L, 3, "w"); bool hasw = !lua_isnil(L, -1); if (hasw) v.w = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
					lua_getfield(L, 3, "a"); bool hasa = !lua_isnil(L, -1); if (!hasw && hasa) v.w = static_cast<float>(luaL_optnumber(L, -1, 0.0)); lua_pop(L,1);
					if (!(hasx||hasr) || !(hasy||hasg) || !(hasz||hasb) || !(hasw||hasa)) { lua_rawgeti(L, 3, 1); v.x = static_cast<float>(luaL_optnumber(L, -1, v.x)); lua_pop(L,1); lua_rawgeti(L, 3, 2); v.y = static_cast<float>(luaL_optnumber(L, -1, v.y)); lua_pop(L,1); lua_rawgeti(L, 3, 3); v.z = static_cast<float>(luaL_optnumber(L, -1, v.z)); lua_pop(L,1); lua_rawgeti(L, 3, 4); v.w = static_cast<float>(luaL_optnumber(L, -1, v.w)); lua_pop(L,1);} 
					prop.set_value(obj, v);
				}
			else {
				spdlog::warn("Unsupported property type for LuaInstance __newindex: {} ({})", key, propValue.get_type().get_name().to_string());
			}
			return 0;
		}
	}

	return 0;
}

void LuaInstance::createInLua(std::shared_ptr<Instance> inst, lua_State* L) {
	LuaInstance* ud = static_cast<LuaInstance*>(lua_newuserdata(L, sizeof(LuaInstance)));
	new (ud) LuaInstance {inst};

	// Ensure metatable exists and is assigned to the userdata
	if (luaL_newmetatable(L, "LuaInstanceMeta")) {
		lua_pushcfunction(L, indexFN);
		lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, newIndexFN);
		lua_setfield(L, -2, "__newindex");
		lua_pushcfunction(L, [](lua_State* L) -> int {
			LuaInstance* ud = static_cast<LuaInstance*>(lua_touserdata(L, 1));
			ud->~LuaInstance();
			return 0;
		});
		lua_setfield(L, -2, "__gc");
	}
	lua_setmetatable(L, -2);
}

LuaInstance& LuaInstance::fromLua(lua_State* L) {
	LuaInstance* ud = static_cast<LuaInstance*>(lua_touserdata(L, 1));
	assert(ud != nullptr && "Expected LuaInstance userdata");
	return *ud;
}