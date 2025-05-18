#pragma once

#include "pch.h"

#include "buffers.h"
#include "camera.h"
#include "shader.h"
#include "renderer.h"

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

		const std::string& getName() const;
		void setName(std::string_view name);

		const std::string& getClassName() const;
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

	class Physics : public Service {
	public:
		Physics() : Service("PhysicsService") {}

		using Ptr = std::shared_ptr<Physics>;
		explicit Physics(std::string_view name) : Service(name) {}

		virtual void onUpdate(float deltaTime) override;
	};

	class LuaManager : public Service {
		sol::state lua;
	public:
		LuaManager() : Service("LuaService") {}
		virtual void onUpdate(float deltaTime) override;
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
			services[service->getName()] = service;
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