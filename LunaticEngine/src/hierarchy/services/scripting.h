#pragma once

#include "../base.h"

namespace Lunatic::Services {
	struct ScriptState {
		bool isRunning = false;
		bool isPaused = false;
		float waitUntil = 0.0f;
		std::string error;
	};

	class Scripting : public Service {
	public:
		Scripting();
		~Scripting() override = default;

		void update(float deltaTime) override;

		void loadScript(const std::string& name, const std::string& code);
		void loadScriptFile(const std::string& name, const std::string& filepath);
		void exec(const std::string& code);
		void execFile(const std::string& filepath);
		void reloadAll();

		std::vector<std::string> getScriptNames() const;
		bool deleteScript(const std::string& name);
		void updateScript(const std::string& name, const std::string& code);
		void updateScriptFromFile(const std::string& name, const std::string& filepath);

		bool getScriptInfo(const std::string& name, ScriptState& outState,
			std::string& outFilepath, bool& outFromFile) const;
		std::string getScriptCode(const std::string& name) const;
		bool isScriptValid(const std::string& name) const;

		void drawImGuiWindow(bool* p_open = nullptr); // Debugging

	private:
		sol::state m_lua;

		float m_currentTime = 0.0f;

		struct ScriptData {
			sol::thread thread;
			sol::environment env;
			sol::coroutine coroutine;
			ScriptState state;
			std::string code;
			std::string filepath;
			bool fromFile = false;
		};

		std::unordered_map<std::string, ScriptData> m_scripts;

		void initializeCoroutineRuntime();
		std::string luaValueToString(const sol::object& obj);
		std::string formatLuaArgs(sol::variadic_args va);
		void registerLogFuncs(sol::environment& env);
		void registerLogFuncsGlobal();
		void runScript(const std::string& name, const std::string& code,
			const std::string& filepath = "", bool fromFile = false);
		static std::string loadFileToString(const std::string& filepath);
	};
} // namespace Lunatic
