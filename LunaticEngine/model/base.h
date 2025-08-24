#pragma once

#include "pch.h"

#include "render/shader.h"

namespace Lunatic {
	class Instance : public std::enable_shared_from_this<Instance> {
	protected:
		std::string name_;
		std::weak_ptr<Instance> parent_;
		std::vector<std::shared_ptr<Instance>> children_;

	public:
		explicit Instance(const std::string_view name);
		virtual ~Instance() = default;

		entt::meta_type metaType;

		std::string_view getName() const;
		void setName(const std::string_view name);

		std::shared_ptr<Instance> getParent() const;
		void setParent(std::shared_ptr<Instance> parent);

		const std::vector<std::shared_ptr<Instance>>& getChildren() const;
		std::shared_ptr<Instance> findChildByName(std::string name) const;

		bool isA(const std::string& className) const {
			auto type = entt::resolve(entt::hashed_string{ className.data() });
			return type == metaType;
		}

		virtual std::string getClassName() const;
		virtual void onAncestorChanged() { /* No-op by default, not always needed */ }
	};

	class Renderable {
	public:
		void setPosition(const glm::vec2& pos) { position = pos; }
		const glm::vec2& getPosition() const { return position; }

		void setScale(const glm::vec2& scl) { scale = scl; }
		const glm::vec2& getScale() const { return scale; }

		void setColor(const glm::vec4& col) { color = col; }
		const glm::vec4& getColor() const { return color; }

		glm::vec2 position = { 0.0f, 0.0f };
		glm::vec2 scale = { 1.0f, 1.0f };
		glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f }; // Default color is white
		float rotation = 0.0f; // In degrees

	public:
		Renderable(); // Registers the Renderable* to the renderer
		~Renderable(); // Unregisters the Renderable* from the renderer

		virtual void draw(Shader& shader) = 0;
	};

	class Updateable {
	public:
		Updateable(); // Registers the Updateable* to the renderer
		~Updateable(); // Unregisters the Updateable* from the renderer

		virtual void update() = 0;
	};
} // namespace Lunatic
