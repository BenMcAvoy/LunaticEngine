#include "pch.h"

#include "camera.h"

using namespace Lunatic;

Camera::Camera(int width, int height) : m_viewportSize(width, height), m_position(0.0f, 0.0f), m_rotation(0.0f), m_zoom(1.0f) {
	updateView();
	updateProjection();
}

void Camera::setPosition(const glm::vec2& position) {
	m_position = position;
	updateView();
}

void Camera::setRotation(float degrees) {
	m_rotation = degrees;
	updateView();
}

void Camera::setZoom(float zoomLevel) {
	m_zoom = glm::clamp(zoomLevel, 0.1f, 100.0f);
	updateProjection();
}

void Camera::translate(const glm::vec2& delta) {
	m_position += delta;
	updateView();
}

void Camera::rotate(float deltaDegrees) {
	m_rotation += deltaDegrees;
	updateView();
}

void Camera::zoom(float deltaZoom) {
	setZoom(m_zoom + deltaZoom);
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
	glm::mat4 transform = glm::mat4(1.0f);
	transform = glm::translate(transform, glm::vec3(-m_position, 0.0f));
	transform = glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation), glm::vec3(0, 0, 1)) * transform;
	m_view = transform;
}

void Camera::updateProjection() {
	float halfWidth = m_viewportSize.x / (m_zoom * 2.0f);
	float halfHeight = m_viewportSize.y / (m_zoom * 2.0f);
	m_projection = glm::ortho(
		-halfWidth, halfWidth,
		-halfHeight, halfHeight,
		-1.0f, 1.0f
	);
}
