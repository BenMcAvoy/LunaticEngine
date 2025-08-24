#include "pch.h"

#include "texture.h"

using namespace Lunatic;

bool Texture::loadFromFile(std::string_view path) {
	std::println("Loading texture from {}", path);

	// Load using stb_image
	stbi_set_flip_vertically_on_load(1); // Flip the image vertically (OGL compliant)
	unsigned char* data = stbi_load(path.data(), &width_, &height_, &channels_, STBI_rgb_alpha);
	channels_ = 4;
	if (!data) {
		std::println("Failed to load texture image: {}", stbi_failure_reason());
		return false;
	}

	glGenTextures(1, &id_);
	glBindTexture(GL_TEXTURE_2D, id_);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(data);
	return true;
}

void Texture::bind(GLuint unit) const {
	if (id_) {
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_2D, id_);
	}
}