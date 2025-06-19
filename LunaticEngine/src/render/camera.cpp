#include "pch.h"

#include "camera.h"

using namespace Lunatic;

Camera::Camera(int width, int height) : m_viewportSize(width, height), m_position(0.0f, 0.0f, 3.0f), m_pitch(0.0f), m_yaw(-90.0f), m_roll(0.0f) {
	updateVectors();
	updateView();
	updateProjection();
}

void Camera::setPosition(const glm::vec3& position) {
	m_position = position;
	updateView();
}

void Camera::setRotation(float pitch, float yaw, float roll) {
	m_pitch = glm::clamp(pitch, -89.0f, 89.0f); // Prevent gimbal lock
	m_yaw = yaw;
	m_roll = roll;
	updateVectors();
	updateView();
}

void Camera::setFOV(float fov) {
	m_fov = glm::clamp(fov, 1.0f, 120.0f);
	updateProjection();
}

void Camera::setNearFar(float nearPlane, float farPlane) {
	m_nearPlane = nearPlane;
	m_farPlane = farPlane;
	updateProjection();
}

void Camera::translate(const glm::vec3& delta) {
	m_position += delta;
	updateView();
}

void Camera::rotate(float deltaPitch, float deltaYaw, float deltaRoll) {
	m_pitch = glm::clamp(m_pitch + deltaPitch, -89.0f, 89.0f);
	m_yaw += deltaYaw;
	m_roll += deltaRoll;
	updateVectors();
	updateView();
}

void Camera::changeFOV(float deltaFOV) {
	setFOV(m_fov + deltaFOV);
}

void Camera::resize(int width, int height) {
	m_viewportSize = { width, height };
	updateProjection();
}

void Camera::setBackgroundColor(const glm::vec3& color) {
	m_backgroundColor = color;
}

const glm::vec3& Camera::getBackgroundColor() const {
	return m_backgroundColor;
}

glm::mat4 Camera::getViewProjection() const {
	return m_projection * m_view;
}

void Camera::updateView() {
	m_view = glm::lookAt(m_position, m_position + m_forward, m_up);
}

void Camera::updateProjection() {
	float aspectRatio = m_viewportSize.x / m_viewportSize.y;
	m_projection = glm::perspective(glm::radians(m_fov), aspectRatio, m_nearPlane, m_farPlane);
}

void Camera::updateVectors() {
	// Calculate the new forward vector
	glm::vec3 forward;
	forward.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
	forward.y = sin(glm::radians(m_pitch));
	forward.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
	m_forward = glm::normalize(forward);
	
	// Calculate right and up vectors
	m_right = glm::normalize(glm::cross(m_forward, m_worldUp));
	m_up = glm::normalize(glm::cross(m_right, m_forward));
	
	if (m_roll != 0.0f) {
		glm::mat4 rollMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(m_roll), m_forward);
		m_right = glm::vec3(rollMatrix * glm::vec4(m_right, 0.0f));
		m_up = glm::vec3(rollMatrix * glm::vec4(m_up, 0.0f));
	}
}
