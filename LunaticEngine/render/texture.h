#pragma once

#include "pch.h"

namespace Lunatic {
	class Texture {
	public:
		bool loadFromFile(std::string_view path);
		void bind(GLuint unit = 0) const;

	private:
		GLuint id_ = 0;
		int width_ = 0;
		int height_ = 0;
		int channels_ = 0;
	};
} // namespace Lunatic
