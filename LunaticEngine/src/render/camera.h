#pragma once

#include "pch.h"

namespace Lunatic {
	class Camera {
	public:
		Camera(int width = 800, int height = 600);

		void setPosition(const glm::vec3& position);
		void setRotation(float pitch, float yaw, float roll = 0.0f);
		void setFOV(float fov);
		void setNearFar(float nearPlane, float farPlane);

		void translate(const glm::vec3& delta);
		void rotate(float deltaPitch, float deltaYaw, float deltaRoll = 0.0f);
		void changeFOV(float deltaFOV);

		void resize(int width, int height);

		void setBackgroundColor(const glm::vec3& color);
		const glm::vec3& getBackgroundColor() const;

		glm::mat4 getViewProjection() const;
		glm::mat4 getView() const { return m_view; }
		glm::mat4 getProjection() const { return m_projection; }
		const glm::vec2& getViewportSize() const { return m_viewportSize; }
		const glm::vec3& getPosition() const { return m_position; }
		const glm::vec3& getForward() const { return m_forward; }
		const glm::vec3& getRight() const { return m_right; }
		const glm::vec3& getUp() const { return m_up; }

	private:
		void updateView();
		void updateProjection();
		void updateVectors();

		glm::vec2 m_viewportSize;
		glm::vec3 m_position;
		float m_pitch = 0.0f; // degrees
		float m_yaw = -90.0f; // degrees (pointing towards -Z initially)
		float m_roll = 0.0f; // degrees
		float m_fov = 80.0f;
		float m_nearPlane = 0.1f;
		float m_farPlane = 100.0f;

		// Camera vectors
		glm::vec3 m_forward;
		glm::vec3 m_right;
		glm::vec3 m_up;
		glm::vec3 m_worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

		glm::vec3 m_backgroundColor = { 0.15f, 0.15f, 0.15f };

		glm::mat4 m_view;
		glm::mat4 m_projection;
	};
} // namespace Lunatic
