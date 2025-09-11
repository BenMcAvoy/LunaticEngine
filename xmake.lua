set_project("Lunatic")

add_requires("glad", {configs = {
	api = "gl=4.6" -- TODO: When cross compiling for WASM, use "gles=3.0"
}})

-- TODO: When cross compiling for WASM, use emscripten-glfw
add_requires("glfw")
add_requires("glm")
add_requires("imgui v1.92.1-docking", {configs = {
	glfw = true,
	opengl3 = true,
	freetype = true
}})
add_requires("luajit")
add_requires("miniaudio")
add_requires("nlohmann_json")
add_requires("rttr")
add_requires("spdlog")
add_requires("stb")
add_requires("box2d") -- TODO: look into `avx2` config (could potentially improve performance on supported hardware)

set_languages("c++20")

add_rules("mode.debug", "mode.release")
add_includedirs("LunaticEngine/")

-- Static library target
target("LunaticEngine")
    set_kind("static")
	set_pcxxheader("LunaticEngine/pch.h")
    add_files("LunaticEngine/**.cpp")
    add_includedirs("LunaticEngine", {public = true})
	add_packages("glad", "glfw", "glm", "imgui", "luajit", "miniaudio", "nlohmann_json", "rttr", "spdlog", "stb", "box2d")

-- Runtime target
target("LunaticRuntime")
    set_kind("binary")
    add_files("LunaticRuntime/**.cpp")
    add_deps("LunaticEngine")
    add_includedirs("LunaticRuntime")
	add_packages("glad", "glfw", "glm", "imgui", "luajit", "miniaudio", "nlohmann_json", "rttr", "spdlog", "stb", "box2d")

-- Editor target
target("LunaticEditor")
    set_kind("binary")
	set_pcxxheader("LunaticEditor/pch.h")
    add_files("LunaticEditor/**.cpp")
    add_deps("LunaticEngine")
    add_includedirs("LunaticEditor")
	add_packages("glad", "glfw", "glm", "imgui", "luajit", "miniaudio", "nlohmann_json", "rttr", "spdlog", "stb", "box2d")
