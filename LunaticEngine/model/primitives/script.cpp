#include "pch.h"

#include "script.h"

#include "core/engine.h"

#include "model/primitives/sprite.h"
#include "model/primitives/script.h"

using namespace Lunatic;

Script::Script(std::string_view name) : Instance(name), Updateable() {
    typeInfo = rttr::type::get<Script>();
}

rttr::variant solFNToRTTR(sol::object obj) {
    if (obj.get_type() == sol::type::function) {
        auto fn = obj.as<sol::function>();
		// TODO: This could probably be a std::span
        return std::function<sol::object(const std::vector<sol::object>&)>(
            [fn](const std::vector<sol::object>& args) -> sol::object {
                return fn(sol::as_args(args));
            }
        );
    }

    spdlog::warn("Tried to parse a non-function as function... none");
    return {};
}

rttr::variant solToRTTR(sol::object obj) {
    auto type = obj.get_type();

    switch (type) {
    case sol::type::none:
    case sol::type::lua_nil:
        return {};
    case sol::type::string:
        return obj.as<std::string_view>();
    case sol::type::number:
		return obj.as<float>();
    case sol::type::thread:
        spdlog::warn("Tried to parse a thread... none");
        return {};
    case sol::type::boolean:
        return obj.as<bool>();
    case sol::type::function:
		return solFNToRTTR(obj);
    case sol::type::userdata:
        if (obj.is<glm::vec2>()) {
            auto& vec2 = obj.as<glm::vec2>();
            return vec2;
        }
        else if (obj.is<glm::vec4>()) {
            return obj.as<glm::vec4>();
        }
        else {
            spdlog::warn("Tried to parse an unknown userdata... none");
            return {};
        }
    case sol::type::lightuserdata:
        spdlog::warn("Tried to parse a lightuserdata... none");
        return {};
    case sol::type::table:
        spdlog::warn("Tried to parse a table... none");
        return {};
    case sol::type::poly:
        spdlog::warn("Tried to parse poly... none");
        return {};
    default:
        spdlog::warn("Tried to parse unknown type... none");
        return {};
    };
}

sol::object rttrToSol(sol::state_view lua, const rttr::variant& var, std::string_view context) {
	if (!var.is_valid() || var.is_type<void>())
        return sol::nil;

    rttr::type t = var.get_type();

    if (t == rttr::type::get<int>())
        return sol::make_object(lua, var.to_int());
    else if (t == rttr::type::get<double>())
        return sol::make_object(lua, var.to_double());
    else if (t == rttr::type::get<float>())
        return sol::make_object(lua, static_cast<float>(var.to_double()));
    else if (t == rttr::type::get<bool>())
        return sol::make_object(lua, var.to_bool());
    else if (t == rttr::type::get<std::string>())
        return sol::make_object(lua, var.to_string());
    else if (t.is_pointer() || t.is_class())
    {
		// if it's a ptr to a Lunatic::Instance return as shared_ptr
        if (t == rttr::type::get<std::shared_ptr<Instance>>()) {
            auto& sp = var.get_value<std::shared_ptr<Instance>>();
            if (sp)
                return sol::make_object(lua, sp);
            else
                return sol::nil;
		}

		// if its a vec<shrdptr<Instance>>
        if (t == rttr::type::get<std::vector<std::shared_ptr<Instance>>>()) {
            auto& vec = var.get_value<std::vector<std::shared_ptr<Instance>>>();
			// Create a Lua table
			sol::table luaTable = lua.create_table(static_cast<int>(vec.size()), 0);
            for (size_t i = 0; i < vec.size(); ++i) {
                if (vec[i]) {
                    luaTable[i + 1] = vec[i]; // Lua tables are 1-indexed
                }
                else {
                    luaTable[i + 1] = sol::nil;
                }
            }

			return luaTable;
        }

        // glm::vec2
        if (t == rttr::type::get<glm::vec2>()) {
            auto& vec = var.get_value<glm::vec2>();
            return sol::make_object(lua, vec);
		}

		// glm::vec4
        if (t == rttr::type::get<glm::vec4>()) {
            auto& vec = var.get_value<glm::vec4>();
            return sol::make_object(lua, vec);
		}

		// if it's a std::string_view
        if (t == rttr::type::get<std::string_view>()) {
            auto& sv = var.get_value<std::string_view>();
            return sol::make_object(lua, sv);
		}

        spdlog::warn("ptr/class");
        // For user-defined types, push the raw pointer (or wrap in shared_ptr)
        // todo: proper
        void* ptr = var.get_value<void*>();
        if (ptr)
            return sol::make_object(lua, ptr);
        else
            return sol::nil;
    }

	spdlog::warn("nil, type was {} whilst calling {}", t.get_name().to_string(), context);
    return sol::nil; // fallback
}

void Script::loadCode(std::string path) {
    coroutine_ = sol::coroutine();
    env_ = sol::environment();
    finished_ = false;

    if (!luaInit_) {
        lua_.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string,
			sol::lib::math, sol::lib::table, sol::lib::coroutine, sol::lib::os);

        lua_.set_function("print", [](const std::string& msg) {
            spdlog::info("[LUA] {}", msg);
            });

        auto indexFunc = [](sol::this_state ts, Instance& inst, std::string_view key) -> sol::object {
            auto ss = inst.shared_from_this();
            if (!ss) return sol::nil;

            rttr::type& t = inst.typeInfo;
            rttr::variant obj = std::ref(inst);

            auto prop = t.get_property(key.data());

            if (prop.is_valid()) {
                auto var = prop.get_value(obj);
                if (!var.is_valid()) {
                    spdlog::warn("Invalid variant");
                    return {};
                }
                return rttrToSol(ts, var, key);
            }

            auto method = t.get_method(key.data());
            if (method.is_valid()) {
                auto thunk = [method, obj](sol::this_state ts, sol::variadic_args va) -> sol::object {
                    auto it = va.begin();
                    auto end = va.end();
                    if (it == end) return sol::make_object(ts, sol::nil);
                    ++it; // skip first
                    size_t arg_count = 0;
                    for (auto tmp = it; tmp != end; ++tmp) ++arg_count;

                    std::vector<rttr::argument> args;
                    args.reserve(arg_count);
                    std::vector<rttr::variant> arg_storage;
                    arg_storage.reserve(arg_count);

                    std::vector<std::string> strs;
                    strs.reserve(arg_count);

                    for (; it != end; ++it) {
                        if (it->is<std::string_view>()) {
                            std::string str = it->as<std::string>();
                            spdlog::trace("Pushed string arg: {}", str);
                            strs.emplace_back(str);

                            arg_storage.emplace_back(rttr::variant(std::string_view(strs.back())));
                            args.emplace_back(arg_storage.back());
                        }
                        else {
                            auto res = solToRTTR(*it);
                            if (!res.is_valid()) {
                                spdlog::warn("Invalid argument variant");
                                return sol::make_object(ts, sol::nil);
                            }
                            arg_storage.emplace_back(res);
                            args.emplace_back(arg_storage.back());
                        }
                    }

                    auto ret = method.invoke_variadic(obj, args);
                    if (!ret.is_valid()) return sol::make_object(ts, sol::nil);

                    return rttrToSol(ts, ret, method.get_name().to_string());
                    };
                return sol::make_object(ts, thunk);
            }

			// see if they are trying to access `data` (`LuaUserData`)
            if (key == "data") {
                return sol::make_object(ts, &inst.luaUserData);
			}

			spdlog::warn("No property or method named '{}' in type '{}'", key, t.get_name().to_string());
			return sol::nil;
            };

        auto newIndexFunc = [](sol::this_state ts, Instance& inst, std::string_view key, sol::object value) {
            auto ss = inst.shared_from_this();
            if (!ss) return;
            rttr::type& t = inst.typeInfo;
            rttr::variant obj = std::ref(inst);
            auto prop = t.get_property(key.data());
            if (prop.is_valid()) {
                rttr::variant var = solToRTTR(value);
                if (!var.is_valid()) {
                    spdlog::warn("Invalid variant");
                    return;
                }
                bool success = prop.set_value(obj, var);
                if (!success) {
                    spdlog::warn("Failed to set property '{}'", key);
                }
            }
            else {
                spdlog::warn("No property named '{}' in type '{}'", key, t.get_name().to_string());
            }
			};

        lua_.new_usertype<Instance>("Instance",
            sol::meta_function::index, indexFunc,
			sol::meta_function::new_index, newIndexFunc
        );

        // Bind Engine type (directly, no meta)
        lua_.new_usertype<Engine>("Engine",
            "getInstance", &Engine::getInstance,
            "getMouseX", &Engine::getMouseX,
            "getMouseY", &Engine::getMouseY,
			"getMousePos", &Engine::getMousePos,
			"getMouseButtonState", &Engine::getMouseButtonState,
			"getKeyState", &Engine::getKeyState,
			"drawText", &Engine::drawText,
			"hideCursor", &Engine::hideCursor,
			"showCursor", &Engine::showCursor
		);

        // glm::vec2 binding
        lua_.new_usertype<glm::vec2>("vec2",
            sol::constructors<glm::vec2(), glm::vec2(float, float)>(),
            "x", &glm::vec2::x,
            "y", &glm::vec2::y,
            sol::meta_function::to_string, [](const glm::vec2& v) {
                return "vec2(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ")";
			},
			sol::meta_function::length, [](const glm::vec2&) { return 2; },
			sol::meta_function::addition, sol::overload(
                [](const glm::vec2& a, const glm::vec2& b) { return a + b; },
                [](const glm::vec2& a, float b) { return a + glm::vec2(b); },
                [](float a, const glm::vec2& b) { return glm::vec2(a) + b; }
			),
            sol::meta_function::subtraction, sol::overload(
                [](const glm::vec2& a, const glm::vec2& b) { return a - b; },
                [](const glm::vec2& a, float b) { return a - glm::vec2(b); },
				[](float a, const glm::vec2& b) { return glm::vec2(a) - b; }
            ),
            sol::meta_function::multiplication, sol::overload(
                [](const glm::vec2& a, const glm::vec2& b) { return a * b; },
				[](const glm::vec2& a, float b) { return a * glm::vec2(b); },
                [](float a, const glm::vec2& b) { return glm::vec2(a) * b; }
            ),
            sol::meta_function::division, sol::overload(
                [](const glm::vec2& a, const glm::vec2& b) { return a / b; },
                [](const glm::vec2& a, float b) { return a / glm::vec2(b); },
                [](float a, const glm::vec2& b) { return glm::vec2(a) / b; }
            ),
            sol::meta_function::unary_minus, [](const glm::vec2& v) { return -v; },
            sol::meta_function::equal_to, [](const glm::vec2& a, const glm::vec2& b) { return a == b; },
            "length", [](const glm::vec2& v) { return glm::length(v); },
			"normalize", [](const glm::vec2& v) { return glm::normalize(v); }
		);

        // glm::vec4 binding
        lua_.new_usertype<glm::vec4>("vec4",
            sol::constructors<glm::vec4(), glm::vec4(float, float, float, float)>(),
            "x", &glm::vec4::x,
            "y", &glm::vec4::y,
            "z", &glm::vec4::z,
            "w", &glm::vec4::w
        );

		// LuaUserData binding
        lua_.new_usertype<LuaUserData>("LuaUserData",
            sol::no_constructor,
            sol::meta_function::index, [](LuaUserData& luaData, std::string key) {
                return luaData.get(key);
			},
			sol::meta_function::new_index, [](LuaUserData& luaData, std::string key, sol::stack_object value) {
                luaData.set(key, value);
			},
            sol::meta_function::to_string, [](const LuaUserData&) {
                return "LuaUserData";
            },
			sol::meta_function::length, [](const LuaUserData& luaData) { return luaData.size(); },
			"clear", &LuaUserData::clear
		);

		lua_["yield"] = lua_["coroutine"]["yield"];

        luaInit_ = true;
    }

    std::ifstream file(path.c_str());
    if (!file.is_open()) {
        spdlog::error("Failed to open script file: {}", path);
        return;
	}
    codePath_ = path;
    std::string code((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();

    sol::load_result lr = lua_.load(code, path.c_str());
    if (!lr.valid()) {
        sol::error err = lr;
        spdlog::error("Lua load error in script {}: {}", getName(), err.what());
        finished_ = true;
        return;
    }

    luaThread_ = sol::thread::create(lua_);
    sol::state_view co = luaThread_.state();
    // Create the environment from the thread state
    env_ = sol::environment(co, sol::create, co.globals());
    // Get the loaded function and set env on it
    sol::protected_function fn = lr;
    sol::set_environment(env_, fn);
    // Move the function to the thread
    fn.push();
    lua_xmove(lua_.lua_state(), co.lua_state(), 1);
    // Re-wrap the moved function, and set env
    sol::function cofn(co, -1);
    sol::set_environment(env_, cofn);
	static auto& engine = Engine::getInstance();
	env_["engine"] = &engine;
    // Build the coroutine from the thread's function
    coroutine_ = sol::coroutine(cofn);

    finished_ = false;
}

void Script::reloadCode() {
    if (codePath_.empty()) {
        spdlog::warn("No code path set for script {}, cannot reload", getName());
        return;
    }

    // We should clear all luaDataStores since reloading scripts wipes the backing data
	Engine::getInstance().clearAllLuaDataStores();

    loadCode(codePath_.string());
}

void Script::update() {
	if (codePath_.empty() || finished_) {
		return; // No code to execute
	}

	env_["script"] = static_cast<Instance*>(this);
	static auto& engine = Engine::getInstance();
    env_["root"] = engine.rootInstance.get();

	sol::protected_function_result result = coroutine_();

	if (!result.valid()) {
		sol::error err = result;
		spdlog::error("Lua error in script {}: {}", getName(), err.what());
		finished_ = true;
		return;
	}

	if (result.status() == sol::call_status::yielded) {
		return;
	} else {
		finished_ = true;
	}
}
