#pragma once

#include "../base.h"

namespace Lunatic::Services {
	class Workspace : public Service {
	public:
		Workspace();

		std::vector<std::shared_ptr<Instance>>& getInstances() {
			return children;
		}

		void initialize();

		void update(float deltaTime) override;
		void render() override;
	};
} // namespace Lunatic::Services
