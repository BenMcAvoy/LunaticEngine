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

		void loadCode(std::string_view path);
		std::string_view getCodePath() const {
			return codePath_;
		}

		void reloadCode();

		virtual void update() override;

		static lua_State* getLuaState() {
			return L_;
		}

		RTTR_ENABLE(Instance, Updateable);
		RTTR_REGISTRATION_FRIEND;
	private:
		static inline lua_State* L_ = nullptr;
		static inline bool luaInit_ = false;

		bool finished_ = false;

		std::string codePath_;

		lua_State* co_ = nullptr;
	};
} // namespace Lunatic
