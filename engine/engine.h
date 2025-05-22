#pragma once

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "utils.h"

namespace Lunatic {
class Engine {
   public:
    static Engine& GetInstance();

    Engine(uint32_t width, uint32_t height, std::string_view title);
    ~Engine();

    void run();

   private:
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(Engine&&) = delete;

    uint32_t m_width;
    uint32_t m_height;
    std::string m_title;

    bool m_keys[GLFW_KEY_LAST + 1] = {false};
    bool m_mouseButtons[GLFW_MOUSE_BUTTON_LAST + 1] = {false};
    double m_mouseX = 0.0;
    double m_mouseY = 0.0;
    double m_scrollX = 0.0;
    double m_scrollY = 0.0;

    GLFWwindow* m_window = nullptr;

    static Engine* s_instance;

    // Callbacks (OpenGL)
    static void CB_OnKeyboard(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void CB_OnMouseButton(GLFWwindow* window, int button, int action, int mods);
    static void CB_OnMouseMove(GLFWwindow* window, double xpos, double ypos);
    static void CB_OnMouseScroll(GLFWwindow* window, double xoffset, double yoffset);
    static void CB_OnWindowResize(GLFWwindow* window, int width, int height);
    static void CB_OnError(int error, const char* description);
    static void CB_OnDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
};
}  // namespace Lunatic