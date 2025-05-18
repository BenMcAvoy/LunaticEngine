#pragma once

#include "pch.h"

#include "../camera.h"
#include "../shader.h"

namespace Lunatic {
	struct RenderContext {
		std::shared_ptr<Camera> camera;
		std::shared_ptr<Shader> shader;
	};
}
