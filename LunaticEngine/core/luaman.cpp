#include "pch.h"

#include "luaman.h"

using namespace Lunatic;

LuaMan::LuaMan() {
    lua_.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::math, sol::lib::table);

    lua_.set_function("print", [](const std::string& msg) {
        std::println("Lua: {}", msg);
        });

    lua_["yield"] = lua_["coroutine"]["yield"];
}

void LuaMan::resume() {
	// NOTE: We don't iterate `i` here as we may erase mid-way.
	for (size_t i = 0; i < scripts_.size(); ) {
		sol::protected_function_result result = scripts_[i].co();

		if (!result.valid()) {
			sol::error err = result;
			std::println("Lua error: {}", err.what());
			scripts_.erase(scripts_.begin() + i);
		}
		else if (result.status() == sol::call_status::yielded) {
			++i; // Coroutine yielded, keep it in the list
		}
		else {
			scripts_.erase(scripts_.begin() + i); // Coroutine finished, remove it
		}
	}
}

void LuaMan::loadScript(const std::string& code) {
	// Load the Lua code and handle errors
	sol::load_result lr = lua_.load(code);
    if (!lr.valid()) {
        sol::error err = lr;
        std::println("Lua load error: {}", err.what());
        return;
    }

    sol::function fn = lr; // move the loaded function
	sol::thread thread = sol::thread::create(lua_);

	// Push the script function onto the thread
	fn.push(); lua_xmove(lua_.lua_state(), thread.state(), 1);

	// Create a coroutine from the thread's state
	sol::coroutine co(sol::function(thread.state(), -1));

	// Store the coroutine and thread in the scripts vector
	scripts_.emplace_back(std::move(co), std::move(thread));
}