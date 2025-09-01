#pragma once

#include "pch.h"

#include "model/primitives/sprite.h"

#include <box2d/box2d.h>

namespace Lunatic {
	class PhysicsSprite : public Sprite, public Updateable {
	public:
		PhysicsSprite(std::string_view name);
		~PhysicsSprite();

		static inline b2BodyId lastBodyId_ = { 0, 0 };

		static void stepAll(float timeStep, int subStepCount = 6);

		// Overrides so we can inform box2d of changes (getters are fine as-is)
		void setPosition(const glm::vec2& pos) override;
		void setScale(const glm::vec2& scl) override;
		void setRotation(float rot) override;

		void setBodyType(std::string type);
		std::string getBodyType() const { return bodyType_; }

		// Immediate actions (still available as functions)
		void applyForce(const glm::vec2& force, const glm::vec2& point, bool wake = true) const;
		void applyLinearImpulse(const glm::vec2& impulse, const glm::vec2& point, bool wake = true) const;
		void applyForceToCenter(const glm::vec2& force, bool wake = true) const;
		void applyLinearImpulseToCenter(const glm::vec2& impulse, bool wake = true) const;
		void applyTorque(float torque, bool wake = true) const;

		void setForce(glm::vec2 force) const;
		glm::vec2 getForce() const { return lastForce_; }
		void setForcePoint(glm::vec2 point) const;
		glm::vec2 getForcePoint() const { return lastForcePoint_; }
		void setLinearImpulse(glm::vec2 impulse) const;
		glm::vec2 getLinearImpulse() const { return lastImpulse_; }
		void setImpulsePoint(glm::vec2 point) const;
		glm::vec2 getImpulsePoint() const { return lastImpulsePoint_; }
		void setForceToCenter(glm::vec2 f) const;
		glm::vec2 getForceToCenter() const { return lastForceToCenter_; }
		void setLinearImpulseToCenter(glm::vec2 i) const;
		glm::vec2 getLinearImpulseToCenter() const { return lastImpulseToCenter_; }
		void setTorque(float t) const;
		float getTorque() const { return lastTorque_; }

		void setOnCollisionEnter(std::function<sol::object(const std::vector<sol::object>&)> func) {
			onCollisionEnterCB_ = func;
		}

		void setLinearVelocity(glm::vec2 v) const;
		glm::vec2 getLinearVelocity() const;
		void setAngularVelocity(float w) const; // radians/sec
		float getAngularVelocity() const;
		void setFixedRotation(bool fixed) const;
		bool getFixedRotation() const;

		void setBullet(bool bullet) const;
		bool getBullet() const;

		// Override class name
		std::string_view getClassName() const override {
			return "PhysicsSprite";
		}

		RTTR_ENABLE(Sprite, Updateable);
		RTTR_REGISTRATION_FRIEND;

	private:
		// Static box2d data (e.g. world ID)
		static inline bool box2dDataInitialized_ = false;
		static inline b2WorldId worldId_;

		void update() override;

		b2BodyId bodyId_ = { 0, 0 };
		b2ShapeId shapeId_ = { 0, 0 };

		std::string bodyType_ = "dynamic";

		std::function<sol::object(const std::vector<sol::object>&)> onCollisionEnterCB_;

		mutable glm::vec2 lastForce_{ 0.0f, 0.0f };
		mutable glm::vec2 lastForcePoint_{ 0.0f, 0.0f };
		mutable glm::vec2 lastImpulse_{ 0.0f, 0.0f };
		mutable glm::vec2 lastImpulsePoint_{ 0.0f, 0.0f };
		mutable glm::vec2 lastForceToCenter_{ 0.0f, 0.0f };
		mutable glm::vec2 lastImpulseToCenter_{ 0.0f, 0.0f };
		mutable float lastTorque_{ 0.0f };
		mutable bool lastFixedRotation_{ false };
	};
} // namespace Lunatic
