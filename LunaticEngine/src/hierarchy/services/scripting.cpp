#include "pch.h"

#include "scripting.h"

using namespace Lunatic::Services;

const char* LUA_COROUTINE_SYSTEM = R"CORO(
-- Script runner factory (always wraps the script, basically so that it is in a coroutine)
function create_script_runner(script_code, name)
    name = name or "Script"

    -- Always wrap the code in a function, assume the user isn't stupid
    local wrapped_code = "return function()\n" .. script_code .. "\nend"

    -- Compile the wrapped code
    local chunk, err = loadstring(wrapped_code, name)
    if not chunk then
        error("Compilation error: " .. err)
    end

    -- Execute to get the main function
    local success, main_func = pcall(chunk)
    if not success then
        error("Execution error: " .. main_func)
    end

    if type(main_func) ~= "function" then
        error("Script must return a function")
    end

    -- Create the coroutine
    local co = coroutine.create(main_func)

    -- Return the runner function
    return function()
        if coroutine.status(co) == "dead" then
            return false, 0, nil
        end

        local status, waitTime = coroutine.resume(co)

        if not status then
            return false, 0, waitTime -- err
        end

        if coroutine.status(co) == "suspended" then
            return true, waitTime or 0, nil
        else
            return false, 0, nil
        end
    end
end

-- Standard wait function for use in scripts
function wait(seconds)
    return coroutine.yield(seconds or 0.01)
end
)CORO";

Scripting::Scripting() : Service("Scripting") {
	m_lua.open_libraries(
		sol::lib::base,
		sol::lib::math,
		sol::lib::table,
		sol::lib::string,
		sol::lib::coroutine
	);

	// Initialize system time
	m_currentTime = static_cast<float>(
		std::chrono::duration<double>(
			std::chrono::steady_clock::now().time_since_epoch()
		).count()
		);

	// Set up global logging functions
	registerLogFuncsGlobal();

	// Initialize coroutine runtime
	initializeCoroutineRuntime();
}

void Scripting::initializeCoroutineRuntime() {
	try {
		// Load our coroutine system
		sol::protected_function_result result = m_lua.safe_script(
			LUA_COROUTINE_SYSTEM,
			&sol::script_pass_on_error
		);

		if (!result.valid()) {
			sol::error err = result;
			spdlog::error("[Scripting] Failed to initialize coroutine system: {}", err.what());
		}
	}
	catch (const std::exception& e) {
		spdlog::error("[Scripting] Exception during initialization: {}", e.what());
	}
}

void Scripting::update(float deltaTime) {
	try {
		// Update current time
		m_currentTime = static_cast<float>(
			std::chrono::duration<double>(
				std::chrono::steady_clock::now().time_since_epoch()
			).count()
			);

		// Process each script
		for (auto& [name, script] : m_scripts) {
			// Skip scripts that are not running
			if (!script.state.isRunning) continue;

			// Skip paused scripts
			if (script.state.isPaused) continue;

			// Skip scripts that are waiting
			if (m_currentTime < script.state.waitUntil) continue;

			// Skip invalid coroutines
			if (!script.coroutine.valid()) {
				script.state.isRunning = false;
				script.state.error = "Invalid coroutine";
				continue;
			}

			// Resume the coroutine
			sol::protected_function_result result = script.coroutine();

			if (!result.valid()) {
				// Handle error
				sol::error err = result;
				script.state.isRunning = false;
				script.state.error = err.what();
				spdlog::error("[Scripting][{}] Error: {}", name, err.what());
				continue;
			}

			// Parse results (is_alive, wait_time, error_message)
			bool isAlive = result[0];
			float waitTime = result[1];

			if (result[2].is<std::string>()) {
				// Error occurred
				script.state.isRunning = false;
				script.state.error = result[2].get<std::string>();
				spdlog::error("[Scripting][{}] Error: {}", name, script.state.error);
				continue;
			}

			if (isAlive) {
				// Script is still running, update wait time
				script.state.waitUntil = m_currentTime + waitTime;
			}
			else {
				// Script has finished
				script.state.isRunning = false;
				spdlog::info("[Scripting][{}] Script completed", name);
			}
		}
	}
	catch (std::exception& e) {
		spdlog::error("[Scripting] Update error: {}", e.what());
	}
}

void Scripting::runScript(const std::string& name, const std::string& code,
	const std::string& filepath, bool fromFile) {
	try {
		// Create a new thread for this script
		sol::thread scriptThread = sol::thread::create(m_lua.lua_state());
		// Get a reference to the thread's state
		sol::state_view threadState = scriptThread.state();
		// Create an environment for the script
		sol::environment env(threadState, sol::create, m_lua.globals());

		// Add logging functions to the environment
		registerLogFuncs(env);

		// Create the script runner
		sol::protected_function createRunner = m_lua["create_script_runner"];
		sol::protected_function_result result = createRunner(code, name);

		if (!result.valid()) {
			sol::error err = result;
			spdlog::error("[Scripting][{}] Failed to create script: {}", name, err.what());
			return;
		}

		sol::function runner = result;

		// Store the script data
		m_scripts[name] = ScriptData{
			.thread = std::move(scriptThread),
			.env = std::move(env),
			.coroutine = sol::coroutine(threadState, runner),
			.state = {
				.isRunning = true,
				.isPaused = false,
				.waitUntil = 0.0f,
				.error = ""
			},
			.code = code,
			.filepath = filepath,
			.fromFile = fromFile
		};

		spdlog::info("[Scripting] Successfully loaded script: {}", name);
	}
	catch (const std::exception& e) {
		spdlog::error("[Scripting][{}] Exception during script loading: {}", name, e.what());
	}
}

void Scripting::loadScript(const std::string& name, const std::string& code) {
	runScript(name, code, "", false);
}

void Scripting::loadScriptFile(const std::string& name, const std::string& filepath) {
	std::string code = loadFileToString(filepath);
	if (code.empty()) {
		spdlog::error("[Scripting][{}] Could not load file: {}", name, filepath);
		return;
	}
	runScript(name, code, filepath, true);
}

void Scripting::exec(const std::string& code) {
	sol::protected_function_result result = m_lua.safe_script(code, &sol::script_pass_on_error);
	if (!result.valid()) {
		sol::error err = result;
		spdlog::error("[Scripting] exec error: {}", err.what());
	}
}

void Scripting::execFile(const std::string& filepath) {
	std::string code = loadFileToString(filepath);
	if (code.empty()) {
		spdlog::error("[Scripting] Could not load file: {}", filepath);
		return;
	}
	exec(code);
}

void Scripting::reloadAll() {
	auto backup = m_scripts;
	m_scripts.clear();

	for (const auto& [name, script] : backup) {
		if (script.fromFile && !script.filepath.empty()) {
			loadScriptFile(name, script.filepath);
		}
		else {
			runScript(name, script.code, "", false);
		}
	}
}

std::string Scripting::luaValueToString(const sol::object& obj) {
	switch (obj.get_type()) {
	case sol::type::number:
		return std::to_string(obj.as<double>());
	case sol::type::string:
		return obj.as<std::string>();
	case sol::type::boolean:
		return obj.as<bool>() ? "true" : "false";
	case sol::type::nil:
		return "nil";
	case sol::type::userdata: {
		auto ud = obj.as<sol::userdata>();
		if (ud.is<glm::vec2>()) {
			auto v = ud.as<glm::vec2>();
			return fmt::format("vec2({}, {})", v.x, v.y);
		}
		else if (ud.is<glm::vec3>()) {
			auto v = ud.as<glm::vec3>();
			return fmt::format("vec3({}, {}, {})", v.x, v.y, v.z);
		}
		return "<userdata>";
	}
	case sol::type::table:
		return "<table>";
	default:
		return "<unknown>";
	}
}

std::string Scripting::formatLuaArgs(sol::variadic_args va) {
	std::ostringstream oss;
	for (size_t i = 0; i < va.size(); ++i) {
		oss << luaValueToString(va[i]);
		if (i < va.size() - 1) oss << ", ";
	}
	return oss.str();
}

void Scripting::registerLogFuncs(sol::environment& env) {
	auto format = [this](sol::variadic_args va) -> std::string {
		return formatLuaArgs(va);
		};

	env.set_function("trace", [format](sol::variadic_args va) {
		spdlog::trace("[LUA] {}", format(va));
		});

	env.set_function("debug", [format](sol::variadic_args va) {
		spdlog::debug("[LUA] {}", format(va));
		});

	env.set_function("info", [format](sol::variadic_args va) {
		spdlog::info("[LUA] {}", format(va));
		});

	env.set_function("print", [format](sol::variadic_args va) {
		spdlog::info("[LUA] {}", format(va));
		});

	env.set_function("warn", [format](sol::variadic_args va) {
		spdlog::warn("[LUA] {}", format(va));
		});

	env.set_function("error", [format](sol::variadic_args va) {
		spdlog::error("[LUA] {}", format(va));
		});

	env.set_function("critical", [format](sol::variadic_args va) {
		spdlog::critical("[LUA] {}", format(va));
		});
}

void Scripting::registerLogFuncsGlobal() {
	auto format = [this](sol::variadic_args va) -> std::string {
		return formatLuaArgs(va);
		};

	m_lua.set_function("trace", [format](sol::variadic_args va) {
		spdlog::trace("[LUA] {}", format(va));
		});

	m_lua.set_function("debug", [format](sol::variadic_args va) {
		spdlog::debug("[LUA] {}", format(va));
		});

	m_lua.set_function("info", [format](sol::variadic_args va) {
		spdlog::info("[LUA] {}", format(va));
		});

	m_lua.set_function("print", [format](sol::variadic_args va) {
		spdlog::info("[LUA] {}", format(va));
		});

	m_lua.set_function("warn", [format](sol::variadic_args va) {
		spdlog::warn("[LUA] {}", format(va));
		});

	m_lua.set_function("error", [format](sol::variadic_args va) {
		spdlog::error("[LUA] {}", format(va));
		});

	m_lua.set_function("critical", [format](sol::variadic_args va) {
		spdlog::critical("[LUA] {}", format(va));
		});
}

std::string Scripting::loadFileToString(const std::string& filepath) {
	std::ifstream file(filepath, std::ios::in | std::ios::binary);
	if (!file) return {};
	std::ostringstream contents;
	contents << file.rdbuf();
	return contents.str();
}

std::vector<std::string> Scripting::getScriptNames() const {
	std::vector<std::string> names;
	names.reserve(m_scripts.size());

	for (const auto& [name, _] : m_scripts) {
		names.push_back(name);
	}

	return names;
}

bool Scripting::deleteScript(const std::string& name) {
	return m_scripts.erase(name) > 0;
}

void Scripting::updateScript(const std::string& name, const std::string& code) {
	auto it = m_scripts.find(name);
	if (it != m_scripts.end()) {
		// Keep track of whether it's from a file
		bool fromFile = it->second.fromFile;
		std::string filepath = it->second.filepath;

		// Delete the old script
		m_scripts.erase(it);

		// Run the updated script
		runScript(name, code, filepath, fromFile);
	}
	else {
		// If the script doesn't exist, create it
		loadScript(name, code);
	}
}

void Scripting::updateScriptFromFile(const std::string& name, const std::string& filepath) {
	auto it = m_scripts.find(name);
	if (it != m_scripts.end()) {
		// Delete the old script
		m_scripts.erase(it);
	}

	// Load the script from file
	loadScriptFile(name, filepath);
}

bool Scripting::getScriptInfo(const std::string& name, ScriptState& outState,
	std::string& outFilepath, bool& outFromFile) const {
	auto it = m_scripts.find(name);
	if (it != m_scripts.end()) {
		outState = it->second.state;
		outFilepath = it->second.filepath;
		outFromFile = it->second.fromFile;
		return true;
	}
	return false;
}

std::string Scripting::getScriptCode(const std::string& name) const {
	auto it = m_scripts.find(name);
	if (it != m_scripts.end()) {
		return it->second.code;
	}
	return {};
}

bool Scripting::isScriptValid(const std::string& name) const {
	auto it = m_scripts.find(name);
	if (it != m_scripts.end()) {
		return it->second.coroutine.valid();
	}
	return false;
}

void Scripting::drawImGuiWindow(bool* p_open) {
	if (!ImGui::Begin("Lua Script Manager", p_open)) {
		ImGui::End();
		return;
	}

	static std::string selectedScript;
	static std::string lastSelectedScript;
	static char codeBuffer[16384] = "";
	static char newNameBuffer[256] = "";
	static char newCodeBuffer[8192] = "";
	static char filePathBuffer[512] = "";
	static char execBuffer[4096] = "";

	auto drawScriptList = [&]() {
		ImGui::Text("Loaded Scripts");
		ImGui::Separator();
		auto scriptNames = getScriptNames();

		if (ImGui::BeginListBox("##ScriptList", ImVec2(-FLT_MIN, 150))) {
			for (const auto& name : scriptNames) {
				bool isSelected = (selectedScript == name);
				if (ImGui::Selectable(name.c_str(), isSelected)) {
					selectedScript = name;
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndListBox();
		}
		};

	auto drawScriptActions = [&]() {
		if (selectedScript.empty()) {
			lastSelectedScript.clear();
			return;
		}

		ScriptState state;
		std::string filepath;
		bool fromFile;

		if (!getScriptInfo(selectedScript, state, filepath, fromFile)) return;

		const char* statusText = "Unknown";
		if (state.isRunning)
			statusText = state.isPaused ? "Paused" :
			(m_currentTime < state.waitUntil ? "Waiting" : "Running");
		else
			statusText = state.error.empty() ? "Stopped" : "Error";

		ImGui::Separator();
		ImGui::Text("Script Actions");
		ImGui::Text("Status: %s", statusText);

		if (!state.error.empty()) {
			ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Error: %s", state.error.c_str());
		}

		if (fromFile) {
			ImGui::Text("Source: %s", filepath.c_str());
			if (ImGui::Button("Reload from File")) {
				updateScriptFromFile(selectedScript, filepath);
			}
			ImGui::SameLine();
		}

		auto& script = m_scripts.at(selectedScript);

		if (state.isRunning) {
			if (state.isPaused) {
				if (ImGui::Button("Resume")) script.state.isPaused = false;
			}
			else {
				if (ImGui::Button("Pause")) script.state.isPaused = true;
			}
			ImGui::SameLine();

			if (ImGui::Button("Stop")) {
				script.state.isRunning = false;
			}
			ImGui::SameLine();
		}
		else {
			if (ImGui::Button("Restart")) {
				updateScript(selectedScript, script.code);
			}
			ImGui::SameLine();
		}

		if (ImGui::Button("Delete")) {
			deleteScript(selectedScript);
			selectedScript.clear();
			return;
		}

		if (selectedScript != lastSelectedScript) {
			std::string code = getScriptCode(selectedScript);
			snprintf(codeBuffer, sizeof(codeBuffer), "%s", code.c_str());
			lastSelectedScript = selectedScript;
		}

		ImGui::Text("Script Code:");
		if (ImGui::InputTextMultiline("##CodeEditor", codeBuffer, sizeof(codeBuffer), ImVec2(-FLT_MIN, 200))) {
			// Optional: code changed
		}

		if (ImGui::Button("Update Script")) {
			updateScript(selectedScript, codeBuffer);
		}
		};

	auto drawNewScriptSection = [&]() {
		ImGui::Separator();
		ImGui::Text("Create New Script");

		ImGui::InputText("Name##NewScript", newNameBuffer, sizeof(newNameBuffer));
		ImGui::InputTextMultiline("Code##NewScript", newCodeBuffer, sizeof(newCodeBuffer), ImVec2(-FLT_MIN, 150));

		if (ImGui::Button("Create Script")) {
			if (strlen(newNameBuffer) > 0) {
				loadScript(newNameBuffer, newCodeBuffer);
				newNameBuffer[0] = '\0';
				newCodeBuffer[0] = '\0';
			}
			else {
				ImGui::OpenPopup("ScriptNameError");
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Browse File...")) {
			/*const char* file = tinyfd_openFileDialog(
				"Select Lua Script File",
				"",
				1,
				(const char* []) {
				"*.lua"
			},
				"Lua Files (*.lua)",
				0
			);
			if (file) {
				strncpy(filePathBuffer, file, sizeof(filePathBuffer));
				filePathBuffer[sizeof(filePathBuffer) - 1] = '\0';
			}*/
		}

		ImGui::SameLine();
		ImGui::TextUnformatted(filePathBuffer);

		if (ImGui::Button("Load from File")) {
			if (strlen(newNameBuffer) > 0 && strlen(filePathBuffer) > 0) {
				loadScriptFile(newNameBuffer, filePathBuffer);
				newNameBuffer[0] = '\0';
				filePathBuffer[0] = '\0';
			}
			else if (strlen(newNameBuffer) == 0) {
				ImGui::OpenPopup("ScriptNameError");
			}
			else {
				ImGui::OpenPopup("FilePathError");
			}
		}

		if (ImGui::BeginPopupModal("ScriptNameError", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("Script name cannot be empty!");
			if (ImGui::Button("OK", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}

		if (ImGui::BeginPopupModal("FilePathError", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("File path cannot be empty!");
			if (ImGui::Button("OK", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}
		};

	auto drawQuickExecuteSection = [&]() {
		ImGui::Separator();
		ImGui::Text("Quick Execute");

		ImGui::InputTextMultiline("##ExecCode", execBuffer, sizeof(execBuffer), ImVec2(-FLT_MIN, 100));

		if (ImGui::Button("Execute")) {
			if (strlen(execBuffer) > 0) exec(execBuffer);
		}

		ImGui::SameLine();
		if (ImGui::Button("Browse File to Execute")) {
			// FIXME
		}

		ImGui::SameLine();
		if (ImGui::Button("Execute from File")) {
			if (strlen(filePathBuffer) > 0) {
				execFile(filePathBuffer);
			}
			else {
				ImGui::OpenPopup("FilePathError");
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Reload All Scripts")) {
			reloadAll();
		}
		};

	// ----- Render All Sections -----
	drawScriptList();
	drawScriptActions();
	drawNewScriptSection();
	drawQuickExecuteSection();

	ImGui::End();
}
