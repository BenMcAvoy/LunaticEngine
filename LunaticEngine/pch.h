#pragma once


#define GLFW_INCLUDE_NONE
#define IMGUI_DEFINE_MATH_OPERATORS

#define SOL_ALL_SAFETIES_ON 1
#define SOL_LUAJIT 1

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/trigonometric.hpp>

#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <spdlog/sinks/base_sink.h>
#include <spdlog/spdlog.h>

#include <luajit/lua.hpp>
#include <sol/sol.hpp>

#include <tinyfiledialogs/tinyfiledialogs.h>

#include <stdexcept>
#include <span>
#include <cassert>
#include <string>
#include <format>
#include <cstdint>
#include <vector>
#include <array>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

#define LUN_ASSERT(x, msg) \
	if (!(x)) throw std::runtime_error(std::format("Assertion failed: {} ({}:{})", msg, __FILE__, __LINE__));
