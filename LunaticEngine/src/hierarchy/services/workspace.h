#pragma once

#include "../base.h"

namespace Lunatic::Services {
	class Workspace : public Service, public IRenderable {
	public:
		Workspace() : Service("Workspace") {}

		void update(float deltaTime) override;
		void render() override;
	};
} // namespace Lunatic::Services
