#pragma once

#include "pch.h"

namespace Lunatic {
	class Instance : public std::enable_shared_from_this<Instance> {
	protected:
		std::string name;
		std::string className;

	public:
		std::weak_ptr<Instance> parent;
		std::vector<std::shared_ptr<Instance>> children;

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

		virtual void render() { /* No-op by default */ }

	public:
		glm::vec3 position{ 0.0f, 0.0f, 0.0f };
		glm::vec3 rotation{ 0.0f, 0.0f, 0.0f };
	};

	class InstanceRegistry {
	public:
		using Factory = std::function<std::shared_ptr<Instance>()>;

		static void Register(std::string_view name, Factory factory);
		static std::shared_ptr<Instance> Create(std::string_view name);
	};

	class Service : public Instance {
	public:
		using Ptr = std::shared_ptr<Service>;
		explicit Service(std::string_view name);
		virtual void update(float deltaTime) = 0;
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

			LUN_ASSERT(false, "No found service");
			return nullptr;
		}
	};
} // namespace Lunatic
