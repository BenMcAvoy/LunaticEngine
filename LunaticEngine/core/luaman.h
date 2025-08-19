#pragma once

#include "pch.h"

namespace Lunatic {
	struct LuaScript {
		sol::coroutine co; // co holds chunk
		sol::thread thread;
	};

	class LuaMan {
	public:
		LuaMan();
		~LuaMan() = default;

		// resume all script coroutines
		void resume();

	private:
		void loadScript(const std::string& code);

		sol::state lua_;
		std::vector<LuaScript> scripts_;
	};
} // namespace Lunatic
