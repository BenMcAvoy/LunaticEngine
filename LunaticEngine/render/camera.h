#pragma once

#include "pch.h"

namespace Lunatic {
	class Camera {
	public:
		// 2D orthographic camera. World units are arbitrary; by default the vertical span is 2.0 units at zoom=1.
		Camera(int width = 800, int height = 600);

		void setPosition(const glm::vec3& position);
		// 2D rotation around Z axis (degrees).
		void setRotation(float rotationDegrees);
		void setZoom(float zoom);
		void changeZoom(float deltaZoom);
		void setNearFar(float nearPlane, float farPlane);

		void translate(const glm::vec3& delta);
		// 2D rotation around Z axis (degrees).
		void rotate(float deltaRotationDegrees);

		void resize(int width, int height);

		void setBackgroundColor(const glm::vec3& color);
		const glm::vec3& getBackgroundColor() const;

		glm::mat4 getViewProjection() const;
		glm::mat4 getView() const { return view_; }
		glm::mat4 getProjection() const { return projection_; }
		const glm::vec2& getViewportSize() const { return viewportSize_; }
		const glm::vec3& getPosition() const { return position_; }
		const glm::vec3& getForward() const { return forward_; }
		const glm::vec3& getRight() const { return right_; }
		const glm::vec3& getUp() const { return up_; }

	private:
		void updateView();
		void updateProjection();
		void updateVectors();

		glm::vec2 viewportSize_;
		glm::vec3 position_;
		// 2D rotation around Z axis in degrees
		float rotation_ = 0.0f;
		// Zoom factor: >1 zooms in, <1 zooms out
		float zoom_ = 1.0f;
		float nearPlane_ = -1.0f;
		float farPlane_ = 1.0f;

		glm::vec3 forward_ = { 0.0f, 0.0f, -1.0f };
		glm::vec3 right_ = { 1.0f, 0.0f, 0.0f };
		glm::vec3 up_ = { 0.0f, 1.0f, 0.0f };

		glm::vec3 backgroundColor_ = { 0.15f, 0.15f, 0.15f };

		glm::mat4 view_;
		glm::mat4 projection_;
	};
} // namespace Lunatic
