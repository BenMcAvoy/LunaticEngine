#include "pch.h"

#include "primitives/script.h"
#include "primitives/sprite.h"
#include "primitives/camera.h"
#include "primitives/nativescript.h"
#include "primitives/sound.h"
#include "primitives/physicssprite.h"

extern "C" void _force_link_anchor() {}

#define CONSTRUCT_IMPL(ClassName) \
.method("construct", [](const std::string_view name) { \
    auto ptr = std::make_shared<Lunatic::ClassName>(name); \
    return std::static_pointer_cast<Lunatic::Instance>(ptr); \
    })

RTTR_REGISTRATION{
rttr::registration::class_<Lunatic::Instance>("Instance")
.method("construct", [](const std::string_view name) { return std::make_shared<Lunatic::Instance>(name); })
.property("name", &Lunatic::Instance::getName, &Lunatic::Instance::setName)
.property("parent", &Lunatic::Instance::getParent, &Lunatic::Instance::setParent)
.property_readonly("children", &Lunatic::Instance::getChildren)
.property_readonly("className", &Lunatic::Instance::getClassName)
.method("findChildByName", &Lunatic::Instance::findChildByName)
.method("getChildren", &Lunatic::Instance::getChildren)
.method("isA", &Lunatic::Instance::isA)
.method("destroy", &Lunatic::Instance::destroy);

rttr::registration::class_<Lunatic::Renderable>("Renderable")
.property("position", &Lunatic::Renderable::getPosition, &Lunatic::Renderable::setPosition)
.property("scale", &Lunatic::Renderable::getScale, &Lunatic::Renderable::setScale)
.property("color", &Lunatic::Renderable::getColor, &Lunatic::Renderable::setColor)
.property("rotation", &Lunatic::Renderable::getRotation, &Lunatic::Renderable::setRotation);

rttr::registration::class_<Lunatic::Updateable>("Updateable"); // Nothing should be reflected here

rttr::registration::class_<Lunatic::Sprite>("Sprite")
CONSTRUCT_IMPL(Sprite)
.property("texturePath", &Lunatic::Sprite::getTexturePath, &Lunatic::Sprite::setTexture)
.method("clearTexture", &Lunatic::Sprite::clearTexture);

rttr::registration::class_<Lunatic::Script>("Script")
CONSTRUCT_IMPL(Script)
.constructor<std::string_view>()
.property("codePath", &Lunatic::Script::getCodePath, &Lunatic::Script::loadCode);

rttr::registration::class_<Lunatic::NativeScript>("NativeScript"); // Nothing should be reflected here

rttr::registration::class_<Lunatic::Camera>("Camera")
CONSTRUCT_IMPL(Camera)
.property_readonly("viewportSize", &Lunatic::Camera::viewportSize_)
.property("rotation", &Lunatic::Camera::rotation_)
.property("zoom", &Lunatic::Camera::getZoom, &Lunatic::Camera::setZoom)
.property("nearPlane", &Lunatic::Camera::getNearPlane, &Lunatic::Camera::setNearPlane)
.property("farPlane", &Lunatic::Camera::getFarPlane, &Lunatic::Camera::setFarPlane)
.method("screenToWorld", &Lunatic::Camera::screenToWorld)
.method("worldToScreen", &Lunatic::Camera::worldToScreen);

rttr::registration::class_<Lunatic::SoundEmitter>("SoundEmitter")
CONSTRUCT_IMPL(SoundEmitter)
.property("soundPath", &Lunatic::SoundEmitter::getSoundPath, &Lunatic::SoundEmitter::setSoundPath)
.property("looping", &Lunatic::SoundEmitter::isLooping, &Lunatic::SoundEmitter::setLooping)
.property_readonly("isPlaying", &Lunatic::SoundEmitter::isPlaying)
.method("play", &Lunatic::SoundEmitter::play);

rttr::registration::class_<Lunatic::PhysicsSprite>("PhysicsSprite")
CONSTRUCT_IMPL(PhysicsSprite)
.property("bodyType", &Lunatic::PhysicsSprite::getBodyType, &Lunatic::PhysicsSprite::setBodyType)
.property("bullet", &Lunatic::PhysicsSprite::getBullet, &Lunatic::PhysicsSprite::setBullet)
.property("force", &Lunatic::PhysicsSprite::getForce, &Lunatic::PhysicsSprite::setForce)
.property("forcePoint", &Lunatic::PhysicsSprite::getForcePoint, &Lunatic::PhysicsSprite::setForcePoint)
.property("linearImpulse", &Lunatic::PhysicsSprite::getLinearImpulse, &Lunatic::PhysicsSprite::setLinearImpulse)
.property("impulsePoint", &Lunatic::PhysicsSprite::getImpulsePoint, &Lunatic::PhysicsSprite::setImpulsePoint)
.property("forceToCenter", &Lunatic::PhysicsSprite::getForceToCenter, &Lunatic::PhysicsSprite::setForceToCenter)
.property("linearImpulseToCenter", &Lunatic::PhysicsSprite::getLinearImpulseToCenter, &Lunatic::PhysicsSprite::setLinearImpulseToCenter)
.property("torque", &Lunatic::PhysicsSprite::getTorque, &Lunatic::PhysicsSprite::setTorque)
.property("linearVelocity", &Lunatic::PhysicsSprite::getLinearVelocity, &Lunatic::PhysicsSprite::setLinearVelocity)
.property("angularVelocity", &Lunatic::PhysicsSprite::getAngularVelocity, &Lunatic::PhysicsSprite::setAngularVelocity)
.property("fixedRotation", &Lunatic::PhysicsSprite::getFixedRotation, &Lunatic::PhysicsSprite::setFixedRotation)
.method("setOnCollisionEnter", &Lunatic::PhysicsSprite::setOnCollisionEnter);
}
