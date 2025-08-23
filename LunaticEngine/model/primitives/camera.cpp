#include "pch.h"

#include "camera.h"

#include "core/engine.h"

using namespace Lunatic;

// Don't call `Renderable` constructor here to avoid adding to renderables list (the camera shouldn't be rendered traditionally)
Camera::Camera(std::string name) : Instance(std::move(name)) {
	position_ = { 0.0f, 0.0f, 0.0f };
	viewportSize_ = { 800.0f, 600.0f };
	rotation_ = 0.0f;
	zoom_ = 1.0f;
	nearPlane_ = -1.0f;
	farPlane_ = 1.0f;
	update();

	Engine::getInstance().registerMainCamera(this);
}

void Camera::use(Shader& shader) {
	update();
	shader.set("u_viewProjection", projection_ * view_);
}

void Camera::update() {
	// Calculate view_ and projection_ matrices here based on position_, rotation_, zoom_, etc.
	float aspectRatio = viewportSize_.x / viewportSize_.y;
	float orthoHeight = 1.0f / zoom_;
	float orthoWidth = orthoHeight * aspectRatio;
	projection_ = glm::ortho(-orthoWidth, orthoWidth, -orthoHeight, orthoHeight, nearPlane_, farPlane_);
	view_ = glm::mat4(1.0f);
	view_ = glm::translate(view_, -position_);
	view_ = glm::rotate(view_, glm::radians(rotation_), glm::vec3(0.0f, 0.0f, 1.0f));

	// Update forward, right, and up vectors
	forward_ = glm::vec3(0.0f, 0.0f, -1.0f);
	right_ = glm::vec3(1.0f, 0.0f, 0.0f);
	up_ = glm::vec3(0.0f, 1.0f, 0.0f);

	// Apply rotation to right and up vectors
	right_ = glm::rotate(glm::mat4(1.0f), glm::radians(rotation_), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(right_, 0.0f);
	up_ = glm::rotate(glm::mat4(1.0f), glm::radians(rotation_), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(up_, 0.0f);
	right_ = glm::normalize(right_);
	up_ = glm::normalize(up_);
}