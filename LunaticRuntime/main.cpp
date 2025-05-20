#include "core/engine.h"

#include "hierarchy/services/workspace.h"
#include "hierarchy/services/scripting.h"

#include "spdlog/spdlog.h"

int main(void) {
	spdlog::set_level(spdlog::level::trace);

	Lunatic::Engine engine(1280, 720, "Lunatic Engine");

	engine.registerService<Lunatic::Services::Workspace>("Workspace");
	engine.registerService<Lunatic::Services::Scripting>("Scripting");

	engine.run();
}