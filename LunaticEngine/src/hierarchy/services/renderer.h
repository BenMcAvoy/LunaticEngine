#pragma once

#include "hierarchy/base.h"

#include "render/shader.h"
#include "render/camera.h"
#include "render/buffers.h"

namespace Lunatic::Services {
	class Renderer : public Service, public IRenderable {
	public:
		Renderer();
		~Renderer() override = default;

		void update(float deltaTime) override;
		void render() override;

		void resize(int width, int height);

	private:
		Camera m_camera;

		Buffers m_buffers;

		Shader m_shader;
	};
} // namespace Lunatic::Services
