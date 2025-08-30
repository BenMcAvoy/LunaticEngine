#pragma once

#include <glad/glad.h>

namespace Lunatic {
class Framebuffer {
public:
  Framebuffer();
  ~Framebuffer();

  void bind();
  void unbind();

  void resize(int width, int height);

  GLuint texture;
  int width() const { return width_; }
  int height() const { return height_; }

private:
  GLuint fbo_;
  GLuint rbo_;
  int width_ = 800;
  int height_ = 600;
};
} // namespace Lunatic
