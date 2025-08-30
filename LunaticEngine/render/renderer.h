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

		void getViewport(int& vx, int& vy, int& vw, int& vh) const;
		void setViewport(int x, int y, int width, int height);

	private:
		int viewportX_ = 0;
		int viewportY_ = 0;
		int viewportWidth_ = 800;
		int viewportHeight_ = 600;

		std::shared_ptr<RenderState> state_ = nullptr;
		Camera* mainCamera_ = nullptr;
	};
} // namespace Lunatic
