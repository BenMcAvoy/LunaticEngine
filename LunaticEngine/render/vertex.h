#pragma once

namespace Lunatic {
	struct Vertex {
		float position[2]; // x, y
		float texCoord[2]; // u, v
		Vertex(float x, float y, float u = 0.0f, float v = 0.0f) {
			position[0] = x; position[1] = y;
			texCoord[0] = u; texCoord[1] = v;
		}
	};
} // namespace Lunatic
