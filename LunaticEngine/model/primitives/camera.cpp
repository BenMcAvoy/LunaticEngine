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

	metaType = entt::resolve<Camera>();

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

void Camera::resize(int width, int height) {
	viewportSize_ = { static_cast<float>(width), static_cast<float>(height) };
}

glm::vec2 Camera::screenToWorld(const glm::vec2& screenPos) {
	// Convert screen coordinates (0,0 at top-left) to normalized device coordinates (-1 to 1)
	float x = (2.0f * screenPos.x) / viewportSize_.x - 1.0f;
	float y = 1.0f - (2.0f * screenPos.y) / viewportSize_.y; // Invert Y for OpenGL
	glm::vec4 ndcPos = glm::vec4(x, y, 0.0f, 1.0f);
	// Calculate the inverse of the view-projection matrix
	glm::mat4 invVP = glm::inverse(projection_ * view_);
	// Transform NDC to world coordinates
	glm::vec4 worldPos = invVP * ndcPos;
	if (worldPos.w != 0.0f) {
		worldPos /= worldPos.w; // Perspective divide
	}
	return glm::vec2(worldPos);
}

glm::vec2 Camera::worldToScreen(const glm::vec2& worldPos) {
	// Transform world coordinates to clip space
	glm::vec4 clipSpacePos = projection_ * view_ * glm::vec4(worldPos, 0.0f, 1.0f);
	if (clipSpacePos.w != 0.0f) {
		clipSpacePos /= clipSpacePos.w; // Perspective divide
	}
	// Convert clip space to screen coordinates
	float x = (clipSpacePos.x + 1.0f) / 2.0f * viewportSize_.x;
	float y = (1.0f - (clipSpacePos.y + 1.0f) / 2.0f) * viewportSize_.y; // Invert Y for OpenGL
	return glm::vec2(x, y);
}