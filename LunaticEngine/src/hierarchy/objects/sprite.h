#pragma once

#include "../base.h"

using namespace Lunatic;

class Sprite : public Instance, IRenderable {
public:
	Sprite(std::string_view name) : Instance(name, "Sprite") {}
	~Sprite() override = default;

	void render() override {
		// Implement rendering logic here
	}
};
