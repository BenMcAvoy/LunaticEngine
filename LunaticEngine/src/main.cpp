#include "pch.h"

#include "lunatic/engine.h"

#include "lunatic/datamodel/services/scripting.h"
#include "lunatic/datamodel/services/workspace.h"

int main(void) {
	spdlog::set_level(spdlog::level::trace);

	Lunatic::Engine engine(1280, 720, "Lunatic Engine");

	engine.registerService<Lunatic::Services::Workspace>("Workspace");
	engine.registerService<Lunatic::Services::Scripting>("Scripting");

	engine.run();
}