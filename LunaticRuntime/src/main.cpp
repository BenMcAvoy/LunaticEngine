#include "core/engine.h"

#include "model/base.h"
#include "model/primitives/sprite.h"
#include "model/primitives/script.h"
#include "model/primitives/camera.h"
#include "model/primitives/nativescript.h"

#include <iostream>

template <typename T>
std::shared_ptr<T> AddObjectTo(std::string name, std::shared_ptr<Lunatic::Instance> parent) {
    auto inst = std::make_shared<T>(std::move(name));
    inst->setParent(parent);
    return inst;
}

int main(int argc, char** argv) {
	std::cout << "\033[2J\033[H"; // Clear screen and move cursor to home position

	spdlog::set_level(spdlog::level::trace);

	Lunatic::Engine& engine = Lunatic::Engine::getInstance();
	auto& rootInstance = engine.rootInstance;

	auto camera = AddObjectTo<Lunatic::Camera>("MainCamera", engine.rootInstance);

	auto background = AddObjectTo<Lunatic::Sprite>("Background", rootInstance);
	background->setTexture("paper.png");
	background->setPosition({ 0.0f, 0.0f });
	background->setScale({ 4.0f, 4.0f });

	auto board = AddObjectTo<Lunatic::Instance>("Board", rootInstance);
	for (int x = 0; x < 3; x++) {
		for (int y = 0; y < 3; y++) {
			auto sprite = AddObjectTo<Lunatic::Sprite>("Cell_" + std::to_string(x + 1) + "_" + std::to_string(y + 1), board);
			sprite->setPosition({ (x - 1) * 0.6f, (y - 1) * 0.6f });
			sprite->setScale({ 0.5f, 0.5f });
			sprite->setColor({ 0.0, 0.0, 0.0, 0.1 });
		}
	}

	auto script = AddObjectTo<Lunatic::Script>("GameLogic", board);
	script->loadCode("game.lua");

	// ========================== //

	auto cursor = AddObjectTo<Lunatic::Sprite>("Cursor", rootInstance);
	cursor->setScale({ 0.4f, 0.4f });
	auto cursorScript = AddObjectTo<Lunatic::Script>("CursorScript", cursor);
	cursorScript->loadCode("cursor.lua");

	// ========================== //

	auto reloader = AddObjectTo<Lunatic::NativeScript>("Reloader", rootInstance);
	reloader->setFunction([&](std::shared_ptr<Lunatic::NativeScript> self) {
		if (engine.getKeyState(GLFW_KEY_R) == Lunatic::KeyAction::Pressed) {
			spdlog::info("Reloading scripts...");
			auto script = rootInstance->findChildByName("Board")->findChildByName("GameLogic");
			if (script && script->isA("Script")) {
				std::dynamic_pointer_cast<Lunatic::Script>(script)->reloadCode();
			}
			auto cursor = rootInstance->findChildByName("Cursor")->findChildByName("CursorScript");
			if (cursor && cursor->isA("Script")) {
				std::dynamic_pointer_cast<Lunatic::Script>(cursor)->reloadCode();
			}
		}
		});

	engine.run();
	return 0;
}
