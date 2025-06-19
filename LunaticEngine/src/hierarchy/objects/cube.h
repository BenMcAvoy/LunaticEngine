#pragma once

#include "pch.h"

#include "hierarchy/base.h"

#include "render/buffers.h"

namespace Lunatic {
	class Cube : public Instance {
	public:
		Cube(std::string_view name);
		~Cube() override = default;

		void render() override;

	private:
		static inline std::optional<Buffers> sm_buffers; // Single instance of buffers for all cubes, can be shared.		// Vertices and indices for a full 3D cube with normals
		static constexpr std::array<float, 192> sm_vertices = {
			// Positions          // Normals           // Texture Coords
			// Front face (normal: 0, 0, 1)
			-0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
			 0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
			 0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
			-0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,
			
			// Back face (normal: 0, 0, -1)
			-0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
			-0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
			 0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
			 0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
			
			// Left face (normal: -1, 0, 0)
			-0.5f,  0.5f,  0.5f,  -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
			-0.5f,  0.5f, -0.5f,  -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
			-0.5f, -0.5f, -0.5f,  -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
			-0.5f, -0.5f,  0.5f,  -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
			
			// Right face (normal: 1, 0, 0)
			 0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
			 0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
			 0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
			 0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
			
			// Bottom face (normal: 0, -1, 0)
			-0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
			 0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f, 1.0f, 1.0f,
			 0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
			-0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			
			// Top face (normal: 0, 1, 0)
			-0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
			-0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
			 0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
			 0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f
		};

		static constexpr std::array<std::uint32_t, 36> sm_indices = {
			// Front face
			0, 1, 2,   2, 3, 0,
			// Back face
			4, 5, 6,   6, 7, 4,
			// Left face
			8, 9, 10,  10, 11, 8,
			// Right face
			12, 13, 14, 14, 15, 12,
			// Bottom face
			16, 17, 18, 18, 19, 16,
			// Top face
			20, 21, 22, 22, 23, 20
		};
	};
} // namespace Lunatic
