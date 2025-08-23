#include "pch.h"

#include "renderer.h"

#include "model/base.h"

using namespace Lunatic;

void Renderer::registerMainCamera(Camera* camera) {
	mainCamera_ = camera;
}

void Renderer::render(const std::vector<Renderable*>& renderables) {
	if (!state_) {
		state_ = std::make_shared<RenderState>();
	}

	state_->defaultShader.use();
	mainCamera_->use(state_->defaultShader);

	for (auto* renderable : renderables) {
		auto model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(renderable->position, 0.0f));
		model = glm::rotate(model, glm::radians(renderable->rotation), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(renderable->scale, 1.0f));
		state_->defaultShader.set("u_model", model);
		state_->defaultShader.set("u_colour", renderable->color);
		renderable->draw();
	}
}

void Renderer::resize(int width, int height) {
	mainCamera_->resize(width, height);
	state_->defaultShader.use();
	mainCamera_->use(state_->defaultShader);
}