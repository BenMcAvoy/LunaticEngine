#include "core/engine.h"

#include "model/base.h"
#include "model/primitives/sprite.h"
#include "model/primitives/script.h"
#include "model/primitives/camera.h"

#include "model/reflection.h"

template <typename T>
std::shared_ptr<T> AddObjectTo(std::string name, std::shared_ptr<Lunatic::Instance> parent) {
    auto inst = std::make_shared<T>(std::move(name));
    inst->setParent(parent);
    return inst;
}

int main(int argc, char** argv) {
	Lunatic::Engine& engine = Lunatic::Engine::getInstance();

	AddObjectTo<Lunatic::Camera>("MainCamera", engine.rootInstance);
	AddObjectTo<Lunatic::Sprite>("ChildSprite", engine.rootInstance);
	AddObjectTo<Lunatic::Script>("TestScript", engine.rootInstance->getChildren()[1])->loadCode(R"(
		while true do
			callCount = script:callCounter()
			script:getParent():setPosition({ callCount * 0.001, callCount * 0.001 })
			coroutine.yield()
		end
	)");

	engine.run();
	return 0;
}