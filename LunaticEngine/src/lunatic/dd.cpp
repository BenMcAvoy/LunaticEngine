#include "pch.h"

#include "datamodel.h"

namespace Lunatic {
    // ---------------------------
    // Instance Implementation
    // ---------------------------

    Instance::Instance(std::string_view name, std::string_view className)
        : name(name), className(className) {
    }

    Instance::~Instance() {
        // Clear children so shared_ptrs are released properly
        children.clear();
    }

    void Instance::setParent(std::shared_ptr<Instance> newParent) {
        if (auto oldParent = parent.lock()) {
            oldParent->removeChild(shared_from_this());
        }

        parent = newParent;

        if (newParent) {
            newParent->addChild(shared_from_this());
        }
    }

    std::shared_ptr<Instance> Instance::getParent() const {
        return parent.lock();
    }

    void Instance::addChild(std::shared_ptr<Instance> child) {
        // Avoid duplicates
        if (std::find(children.begin(), children.end(), child) != children.end())
            return;

        children.push_back(child);
        child->parent = shared_from_this();
    }

    void Instance::removeChild(std::shared_ptr<Instance> child) {
        auto it = std::remove(children.begin(), children.end(), child);
        if (it != children.end()) {
            children.erase(it, children.end());
            child->parent.reset();
        }
    }

    std::shared_ptr<Instance> Instance::find(std::string_view name) {
        for (auto& child : children) {
            if (child->getName() == name)
                return child;
        }
        return nullptr;
    }

    std::vector<std::shared_ptr<Instance>> Instance::findAll(std::string_view name) {
        std::vector<std::shared_ptr<Instance>> matches;
        for (auto& child : children) {
            if (child->getName() == name)
                matches.push_back(child);
        }
        return matches;
    }

    std::string_view Instance::getName() {
        return name;
    }

    void Instance::setName(std::string_view newName) {
        name = newName;
    }

    std::string_view Instance::getClassName() {
        return className;
    }

    // ---------------------------
    // InstanceRegistry
    // ---------------------------

    static std::unordered_map<std::string, InstanceRegistry::Factory>& GetRegistry() {
        static std::unordered_map<std::string, InstanceRegistry::Factory> registry;
        return registry;
    }

    void InstanceRegistry::Register(std::string_view name, Factory factory) {
        GetRegistry()[std::string(name)] = std::move(factory);
    }

    std::shared_ptr<Instance> InstanceRegistry::Create(std::string_view name) {
        auto it = GetRegistry().find(std::string(name));
        if (it != GetRegistry().end()) {
            return it->second();
        }
        throw std::runtime_error("InstanceRegistry: Unknown type: " + std::string(name));
    }

    // ---------------------------
    // Sprite Implementation
    // ---------------------------

    Sprite::Sprite()
        : RenderableInstance("Sprite", "Sprite") {
    }

    void Sprite::setPosition(const glm::vec2& pos) {
        position = pos;
    }

    const glm::vec2& Sprite::getPosition() const {
        return position;
    }

    void Sprite::setTexture(std::string_view path) {
        texturePath = path;
    }

    void Sprite::setColour(const glm::vec3& col) {
        colour = col;
    }

    // ---------------------------
    // Workspace Implementation
    // ---------------------------

    void Workspace::onUpdate(float deltaTime) {
        spdlog::trace("Workspace::onUpdate");

        for (auto& child : children) {
            if (auto service = std::dynamic_pointer_cast<Service>(child)) {
                service->onUpdate(deltaTime); // child services
            }
        }
    }

    void Workspace::onRender(const RenderContext& context) {
        spdlog::trace("Workspace::onRender");

        for (auto& child : children) {
            if (auto service = std::dynamic_pointer_cast<RenderableService>(child)) {
                service->onRender(context); // child renderable services
            }
        }
    }

    // ---------------------------
    // LuaService Implementation
    // ---------------------------

    // The Lua code that initializes our coroutine system
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

    -- Execute to get the main function (💔)
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

    LuaManager::LuaManager()
        : Service("LuaManager") {
        // Open standard libraries
        m_lua.open_libraries(
            sol::lib::base,
            sol::lib::package,
            sol::lib::math,
            sol::lib::table,
            sol::lib::string,
            sol::lib::coroutine,
            sol::lib::os
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

    void LuaManager::initializeCoroutineRuntime() {
        try {
            // Load our coroutine system
            sol::protected_function_result result = m_lua.safe_script(
                LUA_COROUTINE_SYSTEM,
                &sol::script_pass_on_error
            );

            if (!result.valid()) {
                sol::error err = result;
                spdlog::error("[LuaManager] Failed to initialize coroutine system: {}", err.what());
            }
        }
        catch (const std::exception& e) {
            spdlog::error("[LuaManager] Exception during initialization: {}", e.what());
        }
    }

    void LuaManager::onUpdate(float deltaTime) {
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
                    spdlog::error("[LuaManager][{}] Error: {}", name, err.what());
                    continue;
                }

                // Parse results (is_alive, wait_time, error_message)
                bool isAlive = result[0];
                float waitTime = result[1];

                if (result[2].is<std::string>()) {
                    // Error occurred
                    script.state.isRunning = false;
                    script.state.error = result[2].get<std::string>();
                    spdlog::error("[LuaManager][{}] Error: {}", name, script.state.error);
                    continue;
                }

                if (isAlive) {
                    // Script is still running, update wait time
                    script.state.waitUntil = m_currentTime + waitTime;
                }
                else {
                    // Script has finished
                    script.state.isRunning = false;
                    spdlog::info("[LuaManager][{}] Script completed", name);
                }
            }
        }
        catch (std::exception& e) {
            spdlog::error("[LuaManager] Update error: {}", e.what());
        }
    }

    void LuaManager::runScript(const std::string& name, const std::string& code,
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
                spdlog::error("[LuaManager][{}] Failed to create script: {}", name, err.what());
                return;
            }

            sol::function runner = result;

            // Store the script data
            m_scripts[name] = ScriptData{
                .thread = std::move(scriptThread),
                .env = std::move(env),
                .mainFunction = sol::function(),  // We don't need this anymore
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

            spdlog::info("[LuaManager] Successfully loaded script: {}", name);
        }
        catch (const std::exception& e) {
            spdlog::error("[LuaManager][{}] Exception during script loading: {}", name, e.what());
        }
    }

    void LuaManager::loadScript(const std::string& name, const std::string& code) {
        runScript(name, code, "", false);
    }

    void LuaManager::loadScriptFile(const std::string& name, const std::string& filepath) {
        std::string code = loadFileToString(filepath);
        if (code.empty()) {
            spdlog::error("[LuaManager][{}] Could not load file: {}", name, filepath);
            return;
        }
        runScript(name, code, filepath, true);
    }

    void LuaManager::exec(const std::string& code) {
        sol::protected_function_result result = m_lua.safe_script(code, &sol::script_pass_on_error);
        if (!result.valid()) {
            sol::error err = result;
            spdlog::error("[LuaManager] exec error: {}", err.what());
        }
    }

    void LuaManager::execFile(const std::string& filepath) {
        std::string code = loadFileToString(filepath);
        if (code.empty()) {
            spdlog::error("[LuaManager] Could not load file: {}", filepath);
            return;
        }
        exec(code);
    }

    void LuaManager::reloadAll() {
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

    std::string LuaManager::luaValueToString(const sol::object& obj) {
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

    std::string LuaManager::formatLuaArgs(sol::variadic_args va) {
        std::ostringstream oss;
        for (size_t i = 0; i < va.size(); ++i) {
            oss << luaValueToString(va[i]);
            if (i < va.size() - 1) oss << ", ";
        }
        return oss.str();
    }

    void LuaManager::registerLogFuncs(sol::environment& env) {
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

    void LuaManager::registerLogFuncsGlobal() {
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

    std::string LuaManager::loadFileToString(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::in | std::ios::binary);
        if (!file) return {};
        std::ostringstream contents;
        contents << file.rdbuf();
        return contents.str();
    }

    std::vector<std::string> LuaManager::getScriptNames() const {
        std::vector<std::string> names;
        names.reserve(m_scripts.size());

        for (const auto& [name, _] : m_scripts) {
            names.push_back(name);
        }

        return names;
    }

    bool LuaManager::deleteScript(const std::string& name) {
        return m_scripts.erase(name) > 0;
    }

    void LuaManager::updateScript(const std::string& name, const std::string& code) {
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

    void LuaManager::updateScriptFromFile(const std::string& name, const std::string& filepath) {
        auto it = m_scripts.find(name);
        if (it != m_scripts.end()) {
            // Delete the old script
            m_scripts.erase(it);
        }

        // Load the script from file
        loadScriptFile(name, filepath);
    }

    bool LuaManager::getScriptInfo(const std::string& name, ScriptState& outState,
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

    std::string LuaManager::getScriptCode(const std::string& name) const {
        auto it = m_scripts.find(name);
        if (it != m_scripts.end()) {
            return it->second.code;
        }
        return {};
    }

    bool LuaManager::isScriptValid(const std::string& name) const {
        auto it = m_scripts.find(name);
        if (it != m_scripts.end()) {
            return it->second.coroutine.valid();
        }
        return false;
    }

    void LuaManager::drawImGuiWindow(bool* p_open) {
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
            } else {
                if (ImGui::Button("Pause")) script.state.isPaused = true;
            }
            ImGui::SameLine();

            if (ImGui::Button("Stop")) {
                script.state.isRunning = false;
            }
            ImGui::SameLine();
        } else {
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
            } else {
                ImGui::OpenPopup("ScriptNameError");
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Browse File...")) {
            const char* file = tinyfd_openFileDialog(
                "Select Lua Script File",
                "",
                1,
                (const char*[]){"*.lua"},
                "Lua Files (*.lua)",
                0
            );
            if (file) {
                strncpy(filePathBuffer, file, sizeof(filePathBuffer));
                filePathBuffer[sizeof(filePathBuffer) - 1] = '\0';
            }
        }

        ImGui::SameLine();
        ImGui::TextUnformatted(filePathBuffer);

        if (ImGui::Button("Load from File")) {
            if (strlen(newNameBuffer) > 0 && strlen(filePathBuffer) > 0) {
                loadScriptFile(newNameBuffer, filePathBuffer);
                newNameBuffer[0] = '\0';
                filePathBuffer[0] = '\0';
            } else if (strlen(newNameBuffer) == 0) {
                ImGui::OpenPopup("ScriptNameError");
            } else {
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
            const char* file = tinyfd_openFileDialog(
                "Select Lua Script to Execute",
                "",
                1,
                (const char*[]){"*.lua"},
                "Lua Files (*.lua)",
                0
            );
            if (file) {
                strncpy(filePathBuffer, file, sizeof(filePathBuffer));
                filePathBuffer[sizeof(filePathBuffer) - 1] = '\0';
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Execute from File")) {
            if (strlen(filePathBuffer) > 0) {
                execFile(filePathBuffer);
            } else {
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

    // ---------------------------
    // PhysicsService Implementation
    // ---------------------------

    void PhysicsManager::onUpdate(float deltaTime) {
        spdlog::trace("PhysicsService::onUpdate");
    }
} // namespace Lunatic


/*#include "scene.h"

using namespace Lunatic;

constexpr float vertexData[] = {
	-0.5f, -0.5f, 0.0f, 0.0f,
	0.5f, -0.5f, 1.0f, 0.0f,
	0.5f,  0.5f, 1.0f, 1.0f,
   -0.5f,  0.5f, 0.0f, 1.0f
};

constexpr std::uint32_t indexData[] = { 0, 1, 2, 2, 3, 0 };

Scene::Scene() {
    rectBuffers.uploadData(
        std::span<const float>(vertexData, std::size(vertexData)),
        std::span<const std::uint32_t>(indexData, std::size(indexData))
    );
    rectBuffers.setAttribute(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)0);
    rectBuffers.setAttribute(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(sizeof(float) * 2));
}

void Scene::addObject(const std::shared_ptr<Object>& object) {
    objects.push_back(object);
    object->parent.reset();
}

void Scene::removeObject(const std::shared_ptr<Object>& obj) {
    std::vector<std::shared_ptr<Object>> stack{ obj };
    std::unordered_set<std::shared_ptr<Object>> toRemove;
    while (!stack.empty()) {
        auto current = stack.back();
        stack.pop_back();
        if (!toRemove.insert(current).second)
            continue;  // already visited
        for (auto& child : current->children) {
            stack.push_back(child);
        }
    }

    if (auto p = obj->parent.lock()) {
        auto& siblings = p->children;
        siblings.erase(
            std::remove_if(siblings.begin(), siblings.end(),
                [&](auto& sp) { return sp == obj; }),
            siblings.end()
        );
    }

    for (auto& rem : toRemove) {
        rem->children.clear();
        rem->parent.reset();
    }

    objects.erase(
        std::remove_if(objects.begin(), objects.end(),
            [&](auto& o) { return toRemove.count(o) != 0; }),
        objects.end()
    );
}*/
