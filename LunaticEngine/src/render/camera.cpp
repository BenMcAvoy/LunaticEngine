#include "pch.h"

#include "camera.h"

using namespace Lunatic;

Camera::Camera() {
  resize(800, 600);
}

void Camera::resize(int width, int height) {
  viewportSize = { width, height };
  recalculateProjection();
}

void Camera::resize(glm::vec2 size) {
  resize(static_cast<int>(size.x), static_cast<int>(size.y));
}

void Camera::update() {
    glm::mat4 transform = glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0, 0, 1));
    transform = glm::translate(transform, -position);
    view = transform;
}

void Camera::setPosition(const glm::vec3& iPosition) {
  position = iPosition;
}

void Camera::setRotation(float iRotation) {
  rotation = iRotation;
}

void Camera::setZoom(float iZoom) {
  zoom = glm::clamp(iZoom, 0.1f, 100.0f); // prevent division by 0 or inversion
  recalculateProjection();
}

void Camera::translate(const glm::vec3& translation) {
  position += translation;
}

void Camera::rotate(float iRotation) {
  rotation += iRotation;
}

void Camera::zoomIn(float deltaZoom) {
  setZoom(zoom + deltaZoom); // reuse clamped setter
}

void Camera::recalculateProjection() {
  float aspect = viewportSize.x / viewportSize.y;
  float halfWidth = (viewportSize.x / zoom) * 0.5f;
  float halfHeight = (viewportSize.y / zoom) * 0.5f;

  projection = glm::ortho(
      -halfWidth, halfWidth,
      -halfHeight, halfHeight,
      -1.0f, 1.0f
  );
}

