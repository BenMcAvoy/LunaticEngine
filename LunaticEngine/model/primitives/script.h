#pragma once

#include "pch.h"

#include "model/base.h"

namespace Lunatic {
	class Script : public Instance, public Updateable {
	public:
		explicit Script(std::string_view name);
		virtual ~Script() = default;

		virtual std::string_view getClassName() const override {
			return "Script";
		}

		void loadCode(std::string path);
		std::string getCodePath() const {
			return codePath_.string();
		}

		void reloadCode();

		virtual void update() override;

		RTTR_ENABLE(Instance, Updateable);
		RTTR_REGISTRATION_FRIEND;
	private:
		static inline sol::state lua_;
		static inline bool luaInit_ = false;

		bool finished_ = false;

		std::filesystem::path codePath_;

		sol::coroutine coroutine_;
		sol::thread luaThread_;
		sol::environment env_;
	};
} // namespace Lunatic
