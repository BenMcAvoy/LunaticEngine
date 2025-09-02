#pragma once

#include "pch.h"

#include "render/shader.h"

namespace Lunatic {
	class LuaUserData {
	public:
		LuaUserData() = default;
		~LuaUserData() = default;

		void set(std::string key, sol::stack_object value);
		sol::object get(std::string key);
		int size() const { return static_cast<int>(entries_.size()); }
		void clear() { entries_.clear(); }

	private:
		std::unordered_map<std::string, sol::object> entries_;
	};

	class Instance : public std::enable_shared_from_this<Instance> {
	protected:
		std::string name_;
		std::weak_ptr<Instance> parent_;
		std::vector<std::shared_ptr<Instance>> children_;

	public:
		explicit Instance(const std::string_view name);
		virtual ~Instance() = default;

		rttr::type typeInfo;
		LuaUserData luaUserData;

		std::string_view getName() const;
		void setName(std::string_view name);

		std::shared_ptr<Instance> getParent() const;
		void setParent(std::shared_ptr<Instance> parent);

		const std::vector<std::shared_ptr<Instance>>& getChildren() const;
		std::shared_ptr<Instance> findChildByName(std::string_view name) const;

		bool isA(std::string_view className) const {
			rttr::type other = rttr::type::get_by_name(className.data());
			return typeInfo.is_derived_from(other);
		}

		virtual std::string_view getClassName() const;
		virtual void onAncestorChanged() { /* No-op by default, not always needed */ }

		nlohmann::json serialize() const;
		void deserialize(const nlohmann::json& j);

		void clearChildren();

		void destroy();

		RTTR_ENABLE();

		friend class rttr::registration::class_<Instance>; 
	};

	class Renderable {
	public:
		virtual void setPosition(const glm::vec2& pos) { position = pos; }
		const glm::vec2& getPosition() const { return position; }

		virtual void setScale(const glm::vec2& scl) { scale = scl; }
		const glm::vec2& getScale() const { return scale; }

		void setColor(const glm::vec4& col) { color = col; }
		const glm::vec4& getColor() const { return color; }

		virtual void setRotation(float rot) { rotation = rot; }
		float getRotation() const { return rotation; }

		glm::vec2 position = { 0.0f, 0.0f };
		glm::vec2 scale = { 1.0f, 1.0f };
		glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f }; // Default color is white
		float rotation = 0.0f; // In degrees

	public:
		Renderable(); // Registers the Renderable* to the renderer
		~Renderable(); // Unregisters the Renderable* from the renderer

		virtual void draw(Shader& shader) = 0;

		RTTR_ENABLE();
		friend class rttr::registration::class_<Renderable>; 
	};

	class Updateable {
	public:
		Updateable(); // Registers the Updateable* to the renderer
		~Updateable(); // Unregisters the Updateable* from the renderer

		void reflect();

		virtual void update() = 0;

		RTTR_ENABLE();
		friend class rttr::registration::class_<Updateable>; 
	};
} // namespace Lunatic
