#pragma once

#include "hierarchy/base.h"

#include "render/shader.h"
#include "render/camera.h"
#include "render/buffers.h"

namespace Lunatic::Services {	class Renderer : public Service {
	public:
		Renderer();
		~Renderer() override = default;

		void update(float deltaTime) override;
		void render() override;

		void resize(int width, int height);
		// Camera access methods
		Camera& getCamera() { return m_camera; }
		const Camera& getCamera() const { return m_camera; }
	private:
		void updateCameraControls(float deltaTime);

		Camera m_camera;
		Buffers m_buffers;
		Shader m_shader;

		// Camera control state
		bool m_cameraControlEnabled = false;
		bool m_firstMouse = true;
		glm::vec2 m_lastMousePos = { 0.0f, 0.0f };
		float m_cameraSpeed = 5.0f;
		float m_mouseSensitivity = 0.1f;
		float m_pitch = 0.0f;
		float m_yaw = -90.0f;
	};
} // namespace Lunatic::Services
