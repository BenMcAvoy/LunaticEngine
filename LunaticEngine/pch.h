#pragma once

#define GLFW_INCLUDE_NONE

#define IMGUI_DEFINE_MATH_OPERATORS

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <print>
#include <array>
#include <memory>
#include <vector>
#include <random>
#include <fstream>
#include <algorithm>
#include <functional>
#include <filesystem>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <lua.hpp>

#include <sol/sol.hpp>

#include <entt/entt.hpp>

#include <stb_image.h>
