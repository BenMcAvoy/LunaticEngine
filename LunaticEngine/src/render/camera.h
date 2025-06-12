#pragma once

#include "pch.h"

namespace Lunatic {
	class Camera {
	public:
		Camera(int width = 800, int height = 600);

		void setPosition(const glm::vec2& position);
		void setRotation(float degrees);
		void setZoom(float zoomLevel); // zoomLevel is pixels per unit

		void translate(const glm::vec2& delta);
		void rotate(float deltaDegrees);
		void zoom(float deltaZoom);

		void resize(int width, int height);

		void setBackgroundColor(const glm::vec3& color);
		const glm::vec3& getBackgroundColor() const;

		glm::mat4 getViewProjection() const;
		const glm::vec2& getViewportSize() const { return m_viewportSize; }

	private:
		void updateView();
		void updateProjection();

		glm::vec2 m_viewportSize;
		glm::vec2 m_position;
		float m_rotation = 0.0f; // degrees
		float m_zoom = 100.0f;

		glm::vec3 m_backgroundColor = { 0.15f, 0.15f, 0.15f };

		glm::mat4 m_view;
		glm::mat4 m_projection;
	};
} // namespace Lunatic
