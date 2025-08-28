#include "pch.h"

#include "nativescript.h"

using namespace Lunatic;

NativeScript::NativeScript(std::string_view name) : Instance(name), Updateable() {
	typeInfo = rttr::type::get<NativeScript>();
}

void NativeScript::update() {
	if (func_) {
		func_(std::static_pointer_cast<NativeScript>(shared_from_this()));
	}
}
