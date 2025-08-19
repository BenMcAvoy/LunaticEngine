#include "core/engine.h"

#include "model/base.h"
#include "model/primitives/sprite.h"
#include "model/primitives/script.h"

#include "model/reflection.h"

int main(int argc, char** argv) {
	Lunatic::Engine& engine = Lunatic::Engine::getInstance();

	auto child = std::make_shared<Lunatic::Sprite>("ChildSprite");
	child->setParent(engine.rootInstance);

	auto script = std::make_shared<Lunatic::Script>("TestScript");
	script->setParent(child);
	script->loadCode(R"(
		while true do
			callCount = script:callCounter()
			script:getParent():setPosition({ callCount * 0.001, callCount * 0.001 })
			coroutine.yield()
		end
	)");

	engine.run();

	return 0;
}