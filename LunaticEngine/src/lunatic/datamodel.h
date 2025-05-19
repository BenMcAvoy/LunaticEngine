#pragma once

#include "pch.h"

#include "buffers.h"
#include "camera.h"
#include "shader.h"
#include "renderer.h"
#include "utils.h"

#include "defs/rendercontext.h"

namespace Lunatic {
	class Instance : public std::enable_shared_from_this<Instance> {
	protected:
		// Hierarchy
		std::weak_ptr<Instance> parent;
		std::vector<std::shared_ptr<Instance>> children;

		// Names
		std::string name;
		std::string className;

	public:
		explicit Instance(std::string_view name = "", std::string_view className = "Instance");
		virtual ~Instance();

		void setParent(std::shared_ptr<Instance> newParent);
		std::shared_ptr<Instance> getParent() const;

		void addChild(std::shared_ptr<Instance> child);
		void removeChild(std::shared_ptr<Instance> child);
		std::shared_ptr<Instance> find(std::string_view name);
		std::vector<std::shared_ptr<Instance>> findAll(std::string_view name);

		std::string_view getName();
		void setName(std::string_view name);

		std::string_view getClassName();
	};

	class RenderableInstance : public Instance {
	public:
		RenderableInstance(std::string_view name = "", std::string_view className = "RenderableInstance")
			: Instance(name, className) {
		}

		virtual ~RenderableInstance() = default;

		virtual void render(const RenderContext& context) = 0;
	};

	class InstanceRegistry {
	public:
		using Factory = std::function<std::shared_ptr<Instance>()>;

		static void Register(std::string_view name, Factory factory);
		static std::shared_ptr<Instance> Create(std::string_view name);
	};

	class Sprite : public RenderableInstance {
		glm::vec2 position{ 0.0f, 0.0f };

		glm::vec3 colour{ 1.0f, 1.0f, 1.0f };
		std::filesystem::path texturePath{};
	public:
		explicit Sprite();

		void setPosition(const glm::vec2& pos);
		const glm::vec2& getPosition() const;

		void setTexture(std::string_view path);
		void setColour(const glm::vec3& colour);
	};

	class Service : public Instance {
	public:
		using Ptr = std::shared_ptr<Service>;
		explicit Service(std::string_view name) : Instance(name, "Service") {}
		virtual void onUpdate(float deltaTime) = 0;
	};

	class PhysicsManager : public Service {
	public:
		PhysicsManager() : Service("PhysicsService") {}

		using Ptr = std::shared_ptr<PhysicsManager>;
		explicit PhysicsManager(std::string_view name) : Service(name) {}

		virtual void onUpdate(float deltaTime) override;
	};

	struct ScriptState {
		bool isRunning = false;       // Whether the script is currently running
		bool isPaused = false;        // Whether the script is paused
		float waitUntil = 0.0f;       // Time when the script should resume
		std::string error;            // Last error message, if any
	};

	class LuaManager : public Service {
	public:
		LuaManager();
		~LuaManager() override = default;

		void onUpdate(float deltaTime) override;

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

		void drawImGuiWindow(bool* p_open = nullptr);

	private:
		// The core Lua state
		sol::state m_lua;

		// Current system time
		float m_currentTime = 0.0f;

		// Script data structure
		struct ScriptData {
			sol::thread thread;             // Dedicated thread for this script
			sol::environment env;           // Script environment
			sol::function mainFunction;     // Main script function
			sol::coroutine coroutine;       // Coroutine for execution
			ScriptState state;              // Current execution state
			std::string code;               // Script source code
			std::string filepath;           // Source filepath (if from file)
			bool fromFile = false;          // Whether loaded from file
		};

		// Map of script name to script data
		std::unordered_map<std::string, ScriptData> m_scripts;

		// Helper functions
		void initializeCoroutineRuntime();
		std::string luaValueToString(const sol::object& obj);
		std::string formatLuaArgs(sol::variadic_args va);
		void registerLogFuncs(sol::environment& env);
		void registerLogFuncsGlobal();
		void runScript(const std::string& name, const std::string& code,
			const std::string& filepath = "", bool fromFile = false);
		static std::string loadFileToString(const std::string& filepath);
	};

	class RenderableService : public Service {
	public:
		using Ptr = std::shared_ptr<RenderableService>;
		explicit RenderableService(std::string_view name) : Service(name) {}
		virtual void onRender(const RenderContext& context) = 0;
	};

	class Workspace : public RenderableService {
	public:
		Workspace() : RenderableService("Workspace") {}
		void onUpdate(float deltaTime) override;
		void onRender(const RenderContext& context) override;
	};

	class ServiceLocator {
	private:
		static inline std::unordered_map<std::string, std::shared_ptr<Service>> services;

	public:
		template<typename T>
		static void Register(std::shared_ptr<T> service) {
			static_assert(std::is_base_of_v<Service, T>);
			services[std::string(service->getName())] = service;
		}

		template<typename T = Service>
		static std::shared_ptr<T> Get(std::string_view name) {
			auto it = services.find(std::string(name));
			if (it != services.end()) {
				return std::dynamic_pointer_cast<T>(it->second);
			}
			return nullptr;
		}
	};
} // namespace Lunatic