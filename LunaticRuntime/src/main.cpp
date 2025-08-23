#include "core/engine.h"

#include "model/base.h"
#include "model/primitives/sprite.h"
#include "model/primitives/script.h"
#include "model/primitives/camera.h"
#include "model/primitives/nativescript.h"

#include "model/reflection.h"

template <typename T>
std::shared_ptr<T> AddObjectTo(std::string name, std::shared_ptr<Lunatic::Instance> parent) {
    auto inst = std::make_shared<T>(std::move(name));
    inst->setParent(parent);
    return inst;
}

int main(int argc, char** argv) {
	Lunatic::Engine& engine = Lunatic::Engine::getInstance();
	auto rootInstance = engine.rootInstance;

	auto camera = AddObjectTo<Lunatic::Camera>("MainCamera", engine.rootInstance);

	auto boardFolder = AddObjectTo<Lunatic::Instance>("Board", engine.rootInstance);

	// Create 3x3 grid of sprites, (1, 1) being in the middle, (0, 0) bottom-left, (2, 2) top-right
	for (int x = 0; x < 3; x++) {
		for (int y = 0; y < 3; y++) {
			auto sprite = AddObjectTo<Lunatic::Sprite>("Sprite_" + std::to_string(x+1) + "_" + std::to_string(y+1), boardFolder);
			sprite->setPosition({ (x - 1) * 0.6f, (y - 1) * 0.6f });
			sprite->setScale({ 0.5f, 0.5f });
			sprite->setColor({ 0.0, 0.0, 0.0, 1.0 });
		}
	}

	// Load script from `./board.lua`
	auto script = AddObjectTo<Lunatic::Script>("BoardScript", boardFolder);
	auto scriptFile = std::ifstream("board.lua");
	if (scriptFile.is_open()) {
		std::string code((std::istreambuf_iterator<char>(scriptFile)), std::istreambuf_iterator<char>());
		script->loadCode(code);
		scriptFile.close();
	}
	else {
		std::println("Failed to open board.lua");
	}

	auto nativeScript = AddObjectTo<Lunatic::NativeScript>("BoardNativeScript", boardFolder);
	nativeScript->setFunction([](std::shared_ptr<Lunatic::NativeScript> ns) {
		// We try to reload the code of the board script, so we can edit it live
		static auto script = std::dynamic_pointer_cast<Lunatic::Script>(ns->getParent()->findChildByName("BoardScript"));
		if (!script) return;

		static auto lastWriteTime = std::filesystem::file_time_type::min();
		auto currentWriteTime = std::filesystem::last_write_time("board.lua");

		if (currentWriteTime != lastWriteTime) {
			std::println("Reloading board.lua");
			auto scriptFile = std::ifstream("board.lua");
			if (scriptFile.is_open()) {
				std::string code((std::istreambuf_iterator<char>(scriptFile)), std::istreambuf_iterator<char>());
				script->loadCode(code);
				scriptFile.close();
				lastWriteTime = currentWriteTime;
			}
			else {
				std::println("Failed to open board.lua");
			}
		}
		});

	/*auto sprite = AddObjectTo<Lunatic::Sprite>("ChildSprite", engine.rootInstance);
	auto script = AddObjectTo<Lunatic::Script>("TestScript", sprite);

	script->loadCode(R"(
		local camera = root:findChildByName("MainCamera")
		local sprite = script:getParent()
		local mousePos, worldPos

		while true do
			mousePos = engine:getMousePos()
			worldPos = camera:screenToWorld(mousePos)

			sprite:setPosition(worldPos)
			yield()
		end
	)");

	/*script->loadCode(R"(
		local camera = root:findChildByName("MainCamera")
		local sprite = script:getParent()

		while true do
			local w = engine:getKeyState(87) -- W
			local a = engine:getKeyState(65) -- A
			local s = engine:getKeyState(83) -- S
			local d = engine:getKeyState(68) -- D

			local x = d - a
			local y = w - s
			local pos = sprite:getPosition()
			sprite:setPosition(vec2.new(pos.x + x * 0.016, pos.y + y * 0.016))

			yield()
		end
	)");*/

	// Reimplementation of the above ^^
	/*auto nativeScript = AddObjectTo<Lunatic::NativeScript>("NativeScript", sprite);
	nativeScript->setFunction([&engine, rootInstance](std::shared_ptr<Lunatic::NativeScript> ns) {
		static auto camera = std::dynamic_pointer_cast<Lunatic::Camera>(rootInstance->findChildByName("MainCamera"));
		static auto sprite = std::dynamic_pointer_cast<Lunatic::Sprite>(ns->getParent());
		if (!camera || !sprite) return;

		glm::vec2 mousePos = engine.getMousePos();
		glm::vec2 worldPos = camera->screenToWorld(mousePos);
		sprite->setPosition(worldPos);
		});*/

	engine.run();
	return 0;
}