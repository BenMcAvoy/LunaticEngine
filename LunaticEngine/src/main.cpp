#include "pch.h"

#include "lunatic/prelude.h"

int main(void) {
	Lunatic::Engine engine(1280, 720, "Lunatic Engine");

	auto scene = engine.getScene();

	auto object = std::make_shared<Lunatic::Object>();
	auto child = std::make_shared<Lunatic::Object>();
	scene->addObject(object);
	object->attachChild(child);

	engine.run();
}