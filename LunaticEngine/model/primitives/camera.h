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

		glm::vec2 screenToWorld(glm::vec2 screenPos);
		glm::vec2 worldToScreen(glm::vec2 worldPos);

		float getRotation() const { return rotation_; }
		void setRotation(float rot) { rotation_ = rot; update(); }
		float getZoom() const { return zoom_; }
		void setZoom(float zoom) { zoom_ = zoom; update(); }
		float getNearPlane() const { return nearPlane_; }
		void setNearPlane(float nearP) { nearPlane_ = nearP; update(); }
		float getFarPlane() const { return farPlane_; }
		void setFarPlane(float farP) { farPlane_ = farP; update(); }

		virtual std::string_view getClassName() const override {
			return "Camera";
		}

		friend class rttr::registration::class_<Camera>; 

		RTTR_ENABLE(Instance, Renderable);
		RTTR_REGISTRATION_FRIEND
	private:
		void draw(Shader& shader) override { /* No-op */ }

		void update();

		glm::vec2 viewportSize_;
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
