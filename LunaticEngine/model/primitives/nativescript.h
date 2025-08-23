#pragma once

#include "pch.h"

#include "model/base.h"

namespace Lunatic {
	class NativeScript : public Instance, public Updateable {
	public:
		explicit NativeScript(const std::string& name);
		virtual ~NativeScript() = default;

		virtual std::string getClassName() const override {
			return "NativeHook";
		}

		void setFunction(const std::function<void(std::shared_ptr<NativeScript>)>& func) {
			func_ = func;
		}
		
		virtual void update() override;

	private:
		std::function<void(std::shared_ptr<NativeScript>)> func_;
	};
} // namespace Lunatic
