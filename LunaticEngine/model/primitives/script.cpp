#include "pch.h"

#include "script.h"

#include "model/reflection.h"

using namespace Lunatic;

Script::Script(const std::string& name) : Instance(name), Updateable() {
	metaType = entt::resolve<Script>();
}

entt::meta_any luaToENTT(sol::object obj) {
    auto type = obj.get_type();
    switch (type) {
        case sol::type::number:
            return entt::meta_any(obj.as<double>());
        case sol::type::string:
            return entt::meta_any(obj.as<std::string>());
        case sol::type::boolean:
            return entt::meta_any(obj.as<bool>());
        case sol::type::table: {
			auto table = obj.as<sol::table>();
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
			std::println("[reflection] luaToENTT: unsupported type for conversion. type = {}", (int)type);
            return entt::meta_any();
        }
	}
}

sol::object enttToLua(sol::state_view ts, const entt::meta_any& any) {
    if (!any) {
	    std::println("[reflection] enttToLua: any is null");
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
        auto v = any.cast<glm::vec2>();
        sol::table tbl = ts.create_table();
        tbl["x"] = v.x;
        tbl["y"] = v.y;
        return sol::make_object(ts, tbl);
	}
    else if (any.type() == entt::resolve<void>()) {
        return sol::make_object(ts, sol::nil);
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

	// reinterpret_cast instead of static_cast to avoid checks
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
    if (!luaInit_) {
        lua_.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string,
            sol::lib::math, sol::lib::table);

        lua_.set_function("print", [](const std::string& msg) {
            std::println("Lua: {}", msg);
            });

        auto indexFunc = [](sol::this_state ts, Instance& inst, const std::string& key) -> sol::object {
            auto ss = inst.shared_from_this();
            if (!ss) return sol::nil;

            auto thunk = [ss, key](sol::variadic_args sVA) -> sol::object {
                static auto scriptMetaType = entt::resolve<Script>();
                static auto spriteMetaType = entt::resolve<Sprite>();

				std::vector<entt::meta_any> args;
				auto begin = sVA.begin();
				begin++; // skip first arg (self)
				for (auto it = begin; it != sVA.end(); ++it) {
					args.push_back(luaToENTT(*it));
				}

                if (ss->metaType == scriptMetaType) {
					auto res = callMethod<Script>(ss, key, args);
					return enttToLua(sVA.lua_state(), res);
                }
                else if (ss->metaType == spriteMetaType) {
					auto res = callMethod<Sprite>(ss, key, args);
					return enttToLua(sVA.lua_state(), res);
                }
                return sol::nil;
                };
            return sol::make_object(ts, thunk);
            };

        lua_.new_usertype<Instance>("Instance",
            sol::meta_function::index, indexFunc
        );

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
    // Build the coroutine from the thread's function
    coroutine_ = sol::coroutine(cofn);

    code_ = std::string(code);
}

void Script::update() {
	if (code_.empty() || finished_) {
		return; // No code to execute
	}

	std::shared_ptr<Instance> sharedPtr = shared_from_this();
	env_["script"] = sharedPtr;

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
