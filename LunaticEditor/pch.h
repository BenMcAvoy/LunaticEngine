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

#include <spdlog/spdlog.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <rttr/registration>
#include <rttr/type>
#include <rttr/registration_friend>

#include <lua.hpp>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
