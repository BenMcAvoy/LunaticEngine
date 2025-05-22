set_project("LunaticEngine")
set_version("0.1.0")

add_rules("mode.debug", "mode.release")

add_requires("glfw", "spdlog", "glm")
add_requires("glad", {configs = {profile = "core", api = "gl=4.6", extensions = "GL_KHR_debug"}})
add_requires("luajit", {configs = {gc64 = true}})
add_requires("sol2", {configs = {includes_lua = false}})
add_requires("imgui", {version="v1.91.8-docking", configs = {opengl3 = true, glfw = true, freetype = true}})

add_includedirs("engine")
set_languages("c++23")
set_warnings("all")

-- In debug mode, add `LUNATIC_DEBUG` to the compiler flags
if is_mode("debug") then
    add_defines("LUNATIC_DEBUG")
end

target("engine")
    set_kind("static")
    add_files("engine/**.cpp")
    add_headerfiles("engine/**.h")
    add_includedirs("engine", {public = true})
    add_packages("spdlog", "imgui", "glm", "glfw", "glad", "luajit", "sol2", "spdlog")

target("runtime")
    set_kind("binary")
    add_files("runtime/**.cpp")
    add_deps("engine")
    add_packages("spdlog", "imgui", "glm", "glfw", "glad", "luajit", "sol2", "spdlog")

target("editor")
    set_kind("binary")
    add_files("editor/**.cpp")
    add_deps("engine")
    add_packages("spdlog", "imgui", "glm", "glfw", "glad", "luajit", "sol2", "spdlog")
