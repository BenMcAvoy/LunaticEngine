#pragma once

#include "pch.h"

#include "model/base.h"

namespace Lunatic {
	struct PropertyBinding {
		entt::meta_func setter;
		entt::meta_func getter;
	};

	class Script : public Instance, public Updateable {
	public:
		explicit Script(std::string_view name);
		virtual ~Script() = default;

		virtual std::string_view getClassName() const override {
			return "Script";
		}

		void loadCode(std::string_view code);

		virtual void update() override;

	private:
		static inline sol::state lua_;
		static inline bool luaInit_ = false;
		//static inline Utils::HeteroStringMap<PropertyBinding> propertyBindings_;

		bool finished_ = false;
		std::string code_;
		sol::coroutine coroutine_;
		sol::thread luaThread_;
		sol::environment env_;
	};
} // namespace Lunatic
