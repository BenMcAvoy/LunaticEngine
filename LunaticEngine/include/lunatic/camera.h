#pragma once

#include "lunatic/shader.h"

#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Lunatic {
	class Camera {
	public:
		Camera();
		~Camera() = default;

		void setPosition(const glm::vec3& position);
		void setRotation(float rotation);
		void setZoom(float zoom);

		void translate(const glm::vec3& translation);
		void rotate(float rotation);
		void zoomIn(float zoom);

		void resize(int width, int height);
		void resize(glm::vec2 size);

		void update();

		inline glm::mat4 getVP() const { return projection * view; }

	private:
		void recalculateProjection();

		glm::vec2 viewportSize = { 800, 600 };
		glm::vec3 position = glm::vec3(0, 0, 0);
		float rotation = 0.0f; // in degrees
		float zoom = 100.0f;

		glm::mat4 projection;
		glm::mat4 view;

		glm::vec3 background = glm::vec3(0.15, 0.15, 0.15);
	};
} // namespace Lunatic
