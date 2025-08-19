#include "pch.h"

#include "camera.h"

using namespace Lunatic;

Camera::Camera(int width, int height)
		: viewportSize_(width, height),
			position_(0.0f, 0.0f, 0.0f),
			rotation_(0.0f),
			zoom_(1.0f),
			nearPlane_(-1.0f),
			farPlane_(1.0f) {
		updateVectors();
		updateView();
		updateProjection();
}

void Camera::setPosition(const glm::vec3& position) {
	position_ = position;
	updateView();
}

void Camera::setRotation(float rotationDegrees) {
	rotation_ = rotationDegrees;
	updateVectors();
	updateView();
}

void Camera::setZoom(float zoom) {
	zoom_ = glm::max(zoom, 0.0001f);
	updateProjection();
}

void Camera::changeZoom(float deltaZoom) {
	setZoom(zoom_ + deltaZoom);
}

void Camera::setNearFar(float nearPlane, float farPlane) {
	nearPlane_ = nearPlane;
	farPlane_ = farPlane;
	updateProjection();
}

void Camera::translate(const glm::vec3& delta) {
	position_ += delta;
	updateView();
}

void Camera::rotate(float deltaRotationDegrees) {
	rotation_ += deltaRotationDegrees;
	updateVectors();
	updateView();
}


void Camera::resize(int width, int height) {
	viewportSize_ = { width, height };
	updateProjection();
}

void Camera::setBackgroundColor(const glm::vec3& color) {
	backgroundColor_ = color;
}

const glm::vec3& Camera::getBackgroundColor() const {
	return backgroundColor_;
}

glm::mat4 Camera::getViewProjection() const {
	return projection_ * view_;
}

void Camera::updateView() {
	// In 2D, view is a transform by position (x,y) and rotation around Z.
	glm::mat4 translateMat = glm::translate(glm::mat4(1.0f), -position_);
	glm::mat4 rotateMat = glm::rotate(glm::mat4(1.0f), glm::radians(-rotation_), glm::vec3(0.0f, 0.0f, 1.0f));
	view_ = rotateMat * translateMat;
}

void Camera::updateProjection() {
	float w = glm::max(viewportSize_.x, 1.0f);
	float h = glm::max(viewportSize_.y, 1.0f);
	float aspect = w / h;
	// Base vertical half-size; with zoom=1, vertical size = 2.0 units.
	float halfHeight = 1.0f / glm::max(zoom_, 0.0001f);
	float halfWidth = halfHeight * aspect;
	float left = -halfWidth;
	float right = halfWidth;
	float bottom = -halfHeight;
	float top = halfHeight;
	projection_ = glm::ortho(left, right, bottom, top, nearPlane_, farPlane_);
}

void Camera::updateVectors() {
	// In 2D, forward is -Z, up is +Y rotated by Z-rotation, right is +X rotated similarly.
	forward_ = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::mat4 rotZ = glm::rotate(glm::mat4(1.0f), glm::radians(rotation_), glm::vec3(0.0f, 0.0f, 1.0f));
	right_ = glm::normalize(glm::vec3(rotZ * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f)));
	up_ = glm::normalize(glm::vec3(rotZ * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)));
}