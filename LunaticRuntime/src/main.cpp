#include "core/engine.h"

#include "model/base.h"
#include "model/primitives/sprite.h"
#include "model/primitives/script.h"
#include "model/primitives/camera.h"
#include "model/primitives/nativescript.h"
#include "model/primitives/sound.h"
#include "model/primitives/physicssprite.h"

#include <iostream>

int main(int argc, char** argv) {
	std::cout << "\033[2J\033[H"; // Clear screen and move cursor to home position

	spdlog::set_level(spdlog::level::trace);

	Lunatic::Engine& engine = Lunatic::Engine::getInstance();
	engine.init();

	auto& rootInstance = engine.rootInstance;

	if (std::filesystem::exists("../../../../LunaticRuntime/res/scene.json")) {
		std::filesystem::current_path("../../../../LunaticRuntime");
	}

	std::ifstream inFile("res/scene.json");
	if (inFile.is_open()) {
		nlohmann::json sceneJson;
		try {
			inFile >> sceneJson;
			rootInstance->deserialize(sceneJson);
			spdlog::info("Scene deserialized from scene.json");
		}
		catch (const std::exception& e) {
			spdlog::error("Failed to parse scene.json: {}", e.what());
		}
		inFile.close();
	} else {
		spdlog::warn("Could not open res/scene.json, starting with empty scene");
		auto camera = std::make_shared<Lunatic::Camera>("Main Camera");
		camera->setParent(rootInstance);
	}

	engine.run();
}
