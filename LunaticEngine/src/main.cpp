#include "pch.h"

#include "lunatic/engine.h"

int main(void) {
	spdlog::set_level(spdlog::level::trace);

	Lunatic::Engine engine(1280, 720, "Lunatic Engine");

	engine.registerService<Lunatic::Workspace>("Workspace");
	engine.registerService<Lunatic::LuaManager>("LuaManager");
	engine.registerService<Lunatic::Physics>("Physics");

	engine.run();
}