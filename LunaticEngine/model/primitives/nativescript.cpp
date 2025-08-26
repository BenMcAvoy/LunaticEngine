#include "pch.h"

#include "nativescript.h"

using namespace Lunatic;

NativeScript::NativeScript(std::string_view name) : Instance(name), Updateable() {
	metaType = entt::resolve<NativeScript>();
}

void NativeScript::update() {
	if (func_) {
		func_(std::static_pointer_cast<NativeScript>(shared_from_this()));
	}
}
