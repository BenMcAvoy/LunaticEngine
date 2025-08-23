#pragma once

#include "pch.h"

#include "render/shader.h"

#include "model/primitives/camera.h"

namespace Lunatic {
	class Renderable;

	struct RenderState {
		//Camera camera;
		Shader defaultShader;
	};

	class Renderer {
	public:
		Renderer() = default;
		~Renderer() = default;

		void registerMainCamera(Camera* camera);
		Camera* getMainCamera() const { return mainCamera_; }

		void render(const std::vector<Renderable*>&);
		void resize(int width, int height);

		//void registerRenderable(Renderable* renderable);
		//void unregisterRenderable(Renderable* renderable);

		//std::vector<Renderable*>& getRenderables();

	private:
		std::shared_ptr<RenderState> state_ = nullptr;
		Camera* mainCamera_ = nullptr;
	};
} // namespace Lunatic
