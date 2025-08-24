#pragma once

#include "pch.h"

#include "model/base.h"

namespace Lunatic {
	class Script : public Instance, public Updateable {
	public:
		explicit Script(const std::string& name);
		virtual ~Script() = default;

		virtual std::string getClassName() const override {
			return "Script";
		}

		void loadCode(std::string_view code);

		virtual void update() override;

	private:
		static inline sol::state lua_;
		static inline bool luaInit_ = false;

		bool finished_ = false;
		std::string code_;
		sol::coroutine coroutine_;
		sol::thread luaThread_;
		sol::environment env_;
	};
} // namespace Lunatic
