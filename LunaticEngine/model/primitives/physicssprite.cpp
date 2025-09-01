#include "pch.h"

#include "physicssprite.h"
#include "script.h"

using namespace Lunatic;

PhysicsSprite::PhysicsSprite(std::string_view name) : Sprite(name), Updateable() {
	typeInfo = rttr::type::get<PhysicsSprite>();

	// Initialize static box2d data if not already done
	if (!box2dDataInitialized_) {
		b2Vec2 gravity(0.0f, -9.81f);
		b2WorldDef worldDef = b2DefaultWorldDef();
		worldDef.gravity = gravity;
		worldId_ = b2CreateWorld(&worldDef);
		if (!b2World_IsValid(worldId_)) {
			spdlog::error("Failed to create Box2D world");
		} else {
			spdlog::debug("PhysicsSprite static Box2D world created with ID: ({}, {})", worldId_.index1, worldId_.generation);
			box2dDataInitialized_ = true;
		}
	}

	// Create a dynamic body for this PhysicsSprite
	b2BodyDef bodyDef = b2DefaultBodyDef();
	bodyDef.type = b2_dynamicBody;
	bodyDef.position = b2Vec2(position.x, position.y);
	bodyId_ = b2CreateBody(worldId_, &bodyDef);

	if (!b2Body_IsValid(bodyId_)) {
		spdlog::error("Failed to create Box2D body for PhysicsSprite '{}'", name);
	}

	float halfWidth = 0.5f * scale.x;
	float halfHeight = 0.5f * scale.y;
	b2Polygon boxShape = b2MakeBox(halfWidth, halfHeight);
	b2ShapeDef shapeDef = b2DefaultShapeDef();
	shapeDef.density = 1.0f; // default density
	shapeDef.material.friction = 1.3f; // default friction
	// Enable runtime contact/hit events for this shape (applies to dynamic/kinematic bodies)
	shapeDef.enableContactEvents = true;
	shapeDef.enableHitEvents = true;
	shapeId_ = b2CreatePolygonShape(bodyId_, &shapeDef, &boxShape);
	b2Shape_SetUserData(shapeId_, this); // Link shape back to this PhysicsSprite
}

PhysicsSprite::~PhysicsSprite() {
	if (b2Body_IsValid(bodyId_)) {
		if (b2Shape_IsValid(shapeId_)) {
			b2DestroyShape(shapeId_, true);
			shapeId_ = { 0, 0 };
		}
		b2DestroyBody(bodyId_);
		bodyId_ = { 0, 0 };
	}
}

// static
void PhysicsSprite::stepAll(float timeStep, int subStepCount) {
	if (!box2dDataInitialized_) return;

	b2World_Step(worldId_, timeStep, subStepCount);

	b2ContactEvents contactEvents = b2World_GetContactEvents(worldId_);

	std::span<b2ContactBeginTouchEvent> beginEvents(contactEvents.beginEvents, contactEvents.beginCount);
	for (const auto& event : beginEvents) {
		if (!b2Shape_IsValid(event.shapeIdA) || !b2Shape_IsValid(event.shapeIdB)) continue;

		void* userDataA = b2Shape_GetUserData(event.shapeIdA);
		void* userDataB = b2Shape_GetUserData(event.shapeIdB);
		PhysicsSprite* spriteA = static_cast<PhysicsSprite*>(userDataA);
		PhysicsSprite* spriteB = static_cast<PhysicsSprite*>(userDataB);

		static sol::state_view sV = Lunatic::Script::getLuaState();
		static std::vector<sol::object> args;
		args.clear();
		if (spriteA->onCollisionEnterCB_) {
			args.emplace_back(sol::make_object(sV, spriteA->shared_from_this()));
			args.emplace_back(sol::make_object(sV, spriteB->shared_from_this()));
			spriteA->onCollisionEnterCB_(args);
		}
		if (spriteB->onCollisionEnterCB_) {
			args.emplace_back(sol::make_object(sV, spriteB->shared_from_this()));
			args.emplace_back(sol::make_object(sV, spriteA->shared_from_this()));
			spriteB->onCollisionEnterCB_(args);
		}
	}
}

void PhysicsSprite::setPosition(const glm::vec2& pos) {
	// Update render state first
	Sprite::setPosition(pos);

	// Sync physics body transform (if available)
	if (b2Body_IsValid(bodyId_)) {
		const float angleRad = rotation * 0.01745329251994329576923690768489f; // pi/180
		b2Rot rot(cos(angleRad), sin(angleRad));
		b2Body_SetTransform(bodyId_, b2Vec2(pos.x, pos.y), rot);
	} else {
		spdlog::warn("PhysicsSprite::setPosition called but body is invalid");
	}
}

void PhysicsSprite::setScale(const glm::vec2& scl) {
	Sprite::setScale(scl);

	if (!b2Body_IsValid(bodyId_)) {
		spdlog::warn("PhysicsSprite::setScale called but body is invalid");
		return;
	}

	// Rebuild the collider to match the new scale
	if (b2Shape_IsValid(shapeId_)) {
		b2DestroyShape(shapeId_, true);
		shapeId_ = { 0, 0 };
	}

	const float halfWidth = 0.5f * scl.x;
	const float halfHeight = 0.5f * scl.y;
	b2Polygon boxShape = b2MakeBox(halfWidth, halfHeight);
	b2ShapeDef shapeDef = b2DefaultShapeDef();
	shapeDef.density = 1.0f; // keep density consistent
	shapeDef.material.friction = 1.3f;
	shapeDef.enableContactEvents = true;
	shapeDef.enableHitEvents = true;
	shapeId_ = b2CreatePolygonShape(bodyId_, &shapeDef, &boxShape);
	// Re-attach user data so contact events can identify this instance
	b2Shape_SetUserData(shapeId_, this);
	if (!b2Shape_IsValid(shapeId_)) {
		spdlog::error("PhysicsSprite::setScale failed to recreate shape for size ({}, {})", scl.x, scl.y);
	}
}

void PhysicsSprite::setRotation(float rot) {
	Sprite::setRotation(rot);

	if (b2Body_IsValid(bodyId_)) {
		// Fetch current body position to preserve it while updating angle
		const b2Vec2 p = b2Body_GetPosition(bodyId_);
		const float angleRad = rot * 0.01745329251994329576923690768489f; // pi/180
		b2Rot rot(cos(angleRad), sin(angleRad));
		b2Body_SetTransform(bodyId_, p, rot);
	} else {
		spdlog::warn("PhysicsSprite::setRotation called but body is invalid");
	}
}

void PhysicsSprite::setBodyType(std::string type) {
	b2BodyType typeEnum;

	if (type.empty()) {
		spdlog::warn("PhysicsSprite::setBodyType called with empty type, defaulting to dynamic");
		bodyType_ = "dynamic";
		typeEnum = b2_dynamicBody;
	} else {
		char c = std::tolower(type[0]);
		switch (c) {
		case 's':
			bodyType_ = "static";
			typeEnum = b2_staticBody;
			break;
		case 'k':
			bodyType_ = "kinematic";
			typeEnum = b2_kinematicBody;
			break;
		case 'd':
			bodyType_ = "dynamic";
			typeEnum = b2_dynamicBody;
			break;
		default:
			spdlog::warn("PhysicsSprite::setBodyType called with unknown type '{}', defaulting to dynamic", type);
			bodyType_ = "dynamic";
			typeEnum = b2_dynamicBody;
			break;
		}
	}

	if (b2Body_IsValid(bodyId_)) {
		b2Body_SetType(bodyId_, typeEnum);
	} else {
		spdlog::warn("PhysicsSprite::setBodyType called but body is invalid");
	}
}

void PhysicsSprite::update() {
	if (!b2Body_IsValid(bodyId_)) {
		spdlog::warn("PhysicsSprite::update called but body is invalid");
		return;
	}

	// Pull data from Box2D body (this is needed for rendering and scripting sync)
	const b2Vec2 pos = b2Body_GetPosition(bodyId_);
	const b2Rot rot = b2Body_GetRotation(bodyId_);
	// Update position
	position.x = pos.x;
	position.y = pos.y;
	// Convert rotation from radians to degrees
	rotation = std::atan2(rot.s, rot.c) * 57.295779513082320876798154814105f; // 180/pi
	// Note: scale is not updated as Box2D does not handle scaling
}

void PhysicsSprite::applyForce(const glm::vec2& force, const glm::vec2& point, bool wake) const {
	if (b2Body_IsValid(bodyId_)) {
		b2Body_ApplyForce(bodyId_, b2Vec2(force.x, force.y), b2Vec2(point.x, point.y), wake);
	} else {
		spdlog::warn("PhysicsSprite::applyForce called but body is invalid");
	}
}

void PhysicsSprite::applyLinearImpulse(const glm::vec2& impulse, const glm::vec2& point, bool wake) const {
	if (b2Body_IsValid(bodyId_)) {
		b2Body_ApplyLinearImpulse(bodyId_, b2Vec2(impulse.x, impulse.y), b2Vec2(point.x, point.y), wake);
	} else {
		spdlog::warn("PhysicsSprite::applyLinearImpulse called but body is invalid");
	}
}

void PhysicsSprite::applyForceToCenter(const glm::vec2& force, bool wake) const {
	if (b2Body_IsValid(bodyId_)) {
		const b2Vec2 p = b2Body_GetPosition(bodyId_);
		b2Body_ApplyForce(bodyId_, b2Vec2(force.x, force.y), p, wake);
	} else {
		spdlog::warn("PhysicsSprite::applyForceToCenter called but body is invalid");
	}
}

void PhysicsSprite::applyLinearImpulseToCenter(const glm::vec2& impulse, bool wake) const {
	if (b2Body_IsValid(bodyId_)) {
		const b2Vec2 p = b2Body_GetPosition(bodyId_);
		b2Body_ApplyLinearImpulse(bodyId_, b2Vec2(impulse.x, impulse.y), p, wake);
	} else {
		spdlog::warn("PhysicsSprite::applyLinearImpulseToCenter called but body is invalid");
	}
}

void PhysicsSprite::applyTorque(float torque, bool wake) const {
	if (!b2Body_IsValid(bodyId_)) {
		spdlog::warn("PhysicsSprite::applyTorque called but body is invalid");
		return;
	}

	const float halfMax = 0.5f * (scale.x > scale.y ? scale.x : scale.y);
	const float L = (halfMax > 1e-3f) ? halfMax : 0.5f;
	const float fMag = torque / (2.0f * L);
	const b2Vec2 p = b2Body_GetPosition(bodyId_);
	const b2Rot R = b2Body_GetRotation(bodyId_);
	const b2Vec2 xAxis(R.c, R.s);
	const b2Vec2 yAxis(-R.s, R.c);
	const b2Vec2 r = b2Vec2(L * xAxis.x, L * xAxis.y);
	const b2Vec2 F = b2Vec2(fMag * yAxis.x, fMag * yAxis.y);
	b2Body_ApplyForce(bodyId_, F, b2Vec2(p.x + r.x, p.y + r.y), wake);
	b2Body_ApplyForce(bodyId_, b2Vec2(-F.x, -F.y), b2Vec2(p.x - r.x, p.y - r.y), wake);
}

void PhysicsSprite::setLinearVelocity(glm::vec2 v) const {
	if (b2Body_IsValid(bodyId_)) {
		b2Body_SetLinearVelocity(bodyId_, b2Vec2(v.x, v.y));
	} else {
		spdlog::warn("PhysicsSprite::setLinearVelocity called but body is invalid");
	}
}

glm::vec2 PhysicsSprite::getLinearVelocity() const {
	if (b2Body_IsValid(bodyId_)) {
		b2Vec2 v = b2Body_GetLinearVelocity(bodyId_);
		return glm::vec2(v.x, v.y);
	}
	spdlog::warn("PhysicsSprite::getLinearVelocity called but body is invalid");
	return glm::vec2(0.0f);
}

void PhysicsSprite::setAngularVelocity(float w) const {
	if (b2Body_IsValid(bodyId_)) {
		b2Body_SetAngularVelocity(bodyId_, w);
	} else {
		spdlog::warn("PhysicsSprite::setAngularVelocity called but body is invalid");
	}
}

float PhysicsSprite::getAngularVelocity() const {
	if (b2Body_IsValid(bodyId_)) {
		return b2Body_GetAngularVelocity(bodyId_);
	}
	spdlog::warn("PhysicsSprite::getAngularVelocity called but body is invalid");
	return 0.0f;
}

void PhysicsSprite::setFixedRotation(bool fixed) const {
	if (b2Body_IsValid(bodyId_)) {
		b2Body_SetFixedRotation(bodyId_, fixed);
		lastFixedRotation_ = fixed;
	} else {
		spdlog::warn("PhysicsSprite::setFixedRotation called but body is invalid");
	}
}

bool PhysicsSprite::getFixedRotation() const {
	if (b2Body_IsValid(bodyId_)) {
		// Box2D C API doesn't expose a direct getter in this codebase; fallback to cached value
		return lastFixedRotation_;
	}
	spdlog::warn("PhysicsSprite::getFixedRotation called but body is invalid");
	return lastFixedRotation_;
}

void PhysicsSprite::setBullet(bool bullet) const {
	if (b2Body_IsValid(bodyId_)) {
		b2Body_SetBullet(bodyId_, bullet);
	} else {
		spdlog::warn("PhysicsSprite::setBullet called but body is invalid");
	}
}

bool PhysicsSprite::getBullet() const {
	if (b2Body_IsValid(bodyId_)) {
		return b2Body_IsBullet(bodyId_);
	}
	spdlog::warn("PhysicsSprite::getBullet called but body is invalid");
	return false;
}

// Property-style wrappers
void PhysicsSprite::setForce(glm::vec2 force) const {
	lastForce_ = force;
	// use last known point; default is (0,0) in world coords
	applyForce(force, lastForcePoint_, true);
}

void PhysicsSprite::setForcePoint(glm::vec2 point) const {
	lastForcePoint_ = point;
}

void PhysicsSprite::setLinearImpulse(glm::vec2 impulse) const {
	lastImpulse_ = impulse;
	applyLinearImpulse(impulse, lastImpulsePoint_, true);
}

void PhysicsSprite::setImpulsePoint(glm::vec2 point) const {
	lastImpulsePoint_ = point;
}

void PhysicsSprite::setForceToCenter(glm::vec2 f) const {
	lastForceToCenter_ = f;
	applyForceToCenter(f, true);
}

void PhysicsSprite::setLinearImpulseToCenter(glm::vec2 i) const {
	lastImpulseToCenter_ = i;
	applyLinearImpulseToCenter(i, true);
}

void PhysicsSprite::setTorque(float t) const {
	lastTorque_ = t;
	applyTorque(t, true);
}