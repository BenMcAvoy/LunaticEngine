#pragma once

#include "pch.h"

#include "model/base.h"

#include "render/shader.h"

namespace Lunatic {
	class Camera : public Instance, public Renderable {
	public:
		explicit Camera(std::string_view name = "MainCamera");
		~Camera() = default;

		void use(Shader& shader);
		void resize(int width, int height);

		glm::vec2 screenToWorld(const glm::vec2& screenPos);
		glm::vec2 worldToScreen(const glm::vec2& worldPos);

		virtual std::string_view getClassName() const override {
			return "Camera";
		}

	protected:
        void reflect() override;

	private:
		void draw(Shader& shader) override { /* No-op */ }

		void update();

		glm::vec2 viewportSize_;
		glm::vec3 position_;
		float rotation_ = 0.0f;
		float zoom_ = 1.0f;
		float nearPlane_ = -1.0f;
		float farPlane_ = 1.0f;

		glm::vec3 forward_ = { 0.0f, 0.0f, -1.0f };
		glm::vec3 right_ = { 1.0f, 0.0f, 0.0f };
		glm::vec3 up_ = { 0.0f, 1.0f, 0.0f };

		glm::mat4 view_;
		glm::mat4 projection_;
	};
} // namespace Lunatic
