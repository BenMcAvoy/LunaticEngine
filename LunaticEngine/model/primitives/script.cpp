#include "pch.h"

#include "script.h"

#include "model/reflection.h"

#include "core/engine.h"

using namespace Lunatic;

Script::Script(std::string_view name) : Instance(name), Updateable() {
	metaType = entt::resolve<Script>();
}

//#define REFLECTION_DBG

entt::meta_any luaToENTT(sol::object obj) {
    auto type = obj.get_type();
#ifdef REFLECTION_DBG
    std::println("[reflec] luaToENTT({})", (int)type);
#endif
    switch (type) {
        case sol::type::number:
            return entt::meta_any(obj.as<double>());
        case sol::type::string:
            return entt::meta_any(obj.as<std::string_view>());
        case sol::type::boolean:
            return entt::meta_any(obj.as<bool>());
        case sol::type::table: {
			auto table = obj.as<sol::table>();
			std::println("[reflection] luaToENTT: converting table of size {}", table.size());

			// if there are 2 paramers, assume glm::vec2
            if (table.size() == 2) {
                glm::vec2 v;
                v.x = table[1].get_or(0.0f);
                v.y = table[2].get_or(0.0f);
				return entt::meta_any(v);
            }
            else {
				std::println("[reflection] luaToENTT: table size is not 2, cannot convert to glm::vec2 (size = {})", table.size());
            }

            // Handle table conversion if needed
            return entt::meta_any(obj.as<sol::table>());
        }
        default: {
            if (type == sol::type::userdata) {
                if (obj.is<glm::vec2>()) {
                    glm::vec2 vec2 = obj.as<glm::vec2>();
                    return entt::meta_any(vec2);
				}
				else if (obj.is<glm::vec4>()) {
					glm::vec4 vec4 = obj.as<glm::vec4>();
					return entt::meta_any(vec4);
				}
				else if (obj.is<std::shared_ptr<Instance>>()) {
					auto instancePtr = obj.as<std::shared_ptr<Instance>>();
					return entt::meta_any(instancePtr);
				}
				else if (obj.is<std::shared_ptr<Script>>()) {
					auto scriptPtr = obj.as<std::shared_ptr<Script>>();
					return entt::meta_any(scriptPtr);
				}
				else if (obj.is<std::shared_ptr<Sprite>>()) {
					auto spritePtr = obj.as<std::shared_ptr<Sprite>>();
					return entt::meta_any(spritePtr);
				}
				else if (obj.is<std::shared_ptr<Camera>>()) {
					auto cameraPtr = obj.as<std::shared_ptr<Camera>>();
					return entt::meta_any(cameraPtr);
				}
			}

			std::println("[reflection] luaToENTT: unsupported type for conversion. type = {}", (int)type);
            return entt::meta_any();
        }
	}
}

sol::object enttToLua(sol::state_view ts, const entt::meta_any& any) {
    if (!any) {
		std::println("[reflection] enttToLua: any is null, either void or failed call (likely failed call)");
        return sol::make_object(ts, sol::nil);
    }

    // Example for some basic types
    if (any.type() == entt::resolve<int>()) {
        return sol::make_object(ts, any.cast<int>());
    } 
    else if (any.type() == entt::resolve<double>()) {
        return sol::make_object(ts, any.cast<double>());
    } 
    else if (any.type() == entt::resolve<std::string>()) {
        return sol::make_object(ts, any.cast<std::string>());
    }
    else if (any.type() == entt::resolve<std::string_view>()) {
		return sol::make_object(ts, std::string(any.cast<std::string_view>()));
    }
    else if (any.type() == entt::resolve<bool>()) {
        return sol::make_object(ts, any.cast<bool>());
	}
    else if (any.type().info().name() == entt::resolve<std::shared_ptr<Instance>>().info().name()) {
        auto sp = any.cast<std::shared_ptr<Instance>>();
		return sol::make_object(ts, sp);
    }
    else if (any.type().info().name() == entt::resolve<glm::vec2>().info().name()) {
		return sol::make_object(ts, any.cast<glm::vec2>());
	}
    else if (any.type().info().name() == entt::resolve<glm::vec4>().info().name()) {
        return sol::make_object(ts, any.cast<glm::vec4>());
    }
    else if (any.type() == entt::resolve<void>()) {
        return sol::make_object(ts, sol::nil);
	}
    // std::vector support (especially for std::vector<std::shared_ptr<Instance>>)
    else if (any.type().info().name() == entt::resolve<std::vector<std::shared_ptr<Instance>>>().info().name()) {
        auto vec = any.cast<std::vector<std::shared_ptr<Instance>>>();
        sol::table tbl = ts.create_table(static_cast<int>(vec.size()), 0);
        for (size_t i = 0; i < vec.size(); ++i) {
            tbl[i + 1] = vec[i]; // Lua is 1-indexed
        }
        return tbl;
	}
    else {
		std::println("[reflection] enttToLua: unsupported type for conversion. type = {}", any.type().info().name());
        return sol::make_object(ts, any);
    }
}

template<typename T>
entt::meta_any callMethod(std::shared_ptr<Instance> instance, std::string_view methodName, std::vector<entt::meta_any> args) {
	entt::meta_type& mt = instance->metaType;

    entt::meta_func mf = mt.func(entt::hashed_string{ methodName.data() });
    if (!mf) {
        std::println("[reflection] callMethod: method '{}' not found in type '{}'", methodName, mt.info().name());
        return {};
    }

	T& tRef = *reinterpret_cast<T*>(instance.get());

	entt::meta_any result;
    if (args.empty()) {
        result = mf.invoke(tRef);
    } else {
        result = mf.invoke(tRef, args.data(), args.size());
    }
    if (!result) {
        std::println("[reflection] callMethod: invocation failed for method '{}' on type '{}'", methodName, mt.info().name());
        return {};
    }
	return result;
}

void Script::loadCode(std::string_view code) {
    coroutine_ = sol::coroutine();
    env_ = sol::environment();
    finished_ = false;

    if (!luaInit_) {
        lua_.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string,
			sol::lib::math, sol::lib::table, sol::lib::coroutine, sol::lib::os);

        lua_.set_function("print", [](const std::string& msg) {
            std::println("Lua: {}", msg);
            });

        auto indexFunc = [](sol::this_state ts, Instance& inst, std::string_view key) -> sol::object {
            auto ss = inst.shared_from_this();
            if (!ss) return sol::nil;

            auto thunk = [ss, key](sol::variadic_args sVA) -> sol::object {
                static auto scriptMetaType = entt::resolve<Script>();
                static auto spriteMetaType = entt::resolve<Sprite>();
				static auto cameraMetaType = entt::resolve<Camera>();
				static auto instanceMetaType = entt::resolve<Instance>();

				static std::vector<entt::meta_any> args;
                args.clear();

				auto begin = sVA.begin();
				begin++; // skip first arg (self)
				for (auto it = begin; it != sVA.end(); ++it) {
					args.emplace_back(luaToENTT(*it));
				}

                if (ss->metaType == instanceMetaType) {
                    auto res = callMethod<Instance>(ss, key, args);
                    return enttToLua(sVA.lua_state(), res);
                }
                else if (ss->metaType == scriptMetaType) {
					auto res = callMethod<Script>(ss, key, args);
					return enttToLua(sVA.lua_state(), res);
                }
                else if (ss->metaType == spriteMetaType) {
					auto res = callMethod<Sprite>(ss, key, args);
					return enttToLua(sVA.lua_state(), res);
				}
				else if (ss->metaType == cameraMetaType) {
					auto res = callMethod<Camera>(ss, key, args);
					return enttToLua(sVA.lua_state(), res);
                }

				//std::println("[reflection] Unimplemented cast to type {} (was looking for method '{}')", ss->metaType.info().name(), key);
                return sol::nil;
                };
            return sol::make_object(ts, thunk);
            };

        lua_.new_usertype<Instance>("Instance",
            sol::meta_function::index, indexFunc
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
            "y", &glm::vec2::y
		);

        // glm::vec4 binding
        lua_.new_usertype<glm::vec4>("vec4",
            sol::constructors<glm::vec4(), glm::vec4(float, float, float, float)>(),
            "x", &glm::vec4::x,
            "y", &glm::vec4::y,
            "z", &glm::vec4::z,
            "w", &glm::vec4::w
        );

		lua_["yield"] = lua_["coroutine"]["yield"];

        luaInit_ = true;
    }

    sol::load_result lr = lua_.load(code);
    if (!lr.valid()) {
        sol::error err = lr;
        std::println("Lua load error in script {}: {}", getName(), err.what());
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

    code_ = std::string(code);

    finished_ = false;
}

void Script::update() {
	if (code_.empty() || finished_) {
		return; // No code to execute
	}

	std::shared_ptr<Instance> sharedPtr = shared_from_this();
	env_["script"] = sharedPtr;

	static auto& engine = Engine::getInstance();
	env_["root"] = engine.rootInstance;

	sol::protected_function_result result = coroutine_();

	if (!result.valid()) {
		sol::error err = result;
		std::println("Lua error in script {}: {}", getName(), err.what());
		return;
	}

	if (result.status() == sol::call_status::yielded) {
		return;
	} else {
		finished_ = true;
	}
}
