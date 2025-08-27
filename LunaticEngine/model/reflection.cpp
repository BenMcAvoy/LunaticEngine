#include "pch.h"

#include "primitives/script.h"
#include "primitives/sprite.h"
#include "primitives/camera.h"
#include "primitives/nativescript.h"

using namespace Lunatic;
using namespace entt::literals;

void Renderable::reflect() {
    entt::meta_factory<Renderable>()
        .type("Renderable"_hs)
        .func<&Renderable::setPosition>("setPosition"_hs)
        .func<&Renderable::getPosition>("getPosition"_hs)
        .func<&Renderable::setColor>("setColor"_hs)
        .func<&Renderable::getColor>("getColor"_hs);
}

void Updateable::reflect() {
    entt::meta_factory<Updateable>()
        .type("Updateable"_hs)
        .func<&Updateable::update>("update"_hs);
}

void Instance::reflect() {
    entt::meta_factory<Instance>()
        .type("Instance"_hs)
        .func<&Instance::getName>("getName"_hs)
        .func<&Instance::setName>("setName"_hs)
        .func<&Instance::getParent>("getParent"_hs)
        .func<&Instance::setParent>("setParent"_hs)
        .func<&Instance::getChildren>("getChildren"_hs)
        .func<&Instance::getClassName>("getClassName"_hs)
        .func<&Instance::findChildByName>("findChildByName"_hs)
        .func<&Instance::isA>("isA"_hs)
        .data<&Instance::name_>("name"_hs)
        .data<&Instance::parent_>("parent"_hs)
        .data<&Instance::children_>("children"_hs);
}

void Sprite::reflect() {
    entt::meta_factory<Sprite>()
        .type("Sprite"_hs)
        .base<Instance>()
        .base<Renderable>()
        .func<&Sprite::draw>("draw"_hs)
        .func<&Sprite::setTexture>("setTexture"_hs)
        .func<&Sprite::clearTexture>("clearTexture"_hs)
		.data<&Sprite::currentTexturePath_>("currentTexturePath"_hs);
}

void Script::reflect() {
    entt::meta_factory<Script>()
        .type("Script"_hs)
        .base<Instance>()
        .base<Updateable>()
        .func<&Script::loadCode>("loadCode"_hs)
        .func<&Script::update>("update"_hs)
        .data<&Script::code_>("code"_hs);
}

void NativeScript::reflect() {
    entt::meta_factory<NativeScript>()
        .type("NativeScript"_hs)
        .base<Instance>()
        .base<Updateable>()
        .func<&NativeScript::setFunction>("setFunction"_hs)
        .func<&NativeScript::update>("update"_hs);
}

void Camera::reflect() {
    entt::meta_factory<Camera>()
        .type("Camera"_hs)
        .base<Instance>()
        .base<Renderable>()
        .func<&Camera::use>("use"_hs)
        .func<&Camera::resize>("resize"_hs)
        .func<&Camera::screenToWorld>("screenToWorld"_hs)
        .func<&Camera::getPosition>("getPosition"_hs)
        .func<&Camera::setPosition>("setPosition"_hs)
        .data<&Camera::viewportSize_>("viewportSize"_hs)
        .data<&Camera::position_>("position"_hs)
        .data<&Camera::rotation_>("rotation"_hs)
        .data<&Camera::zoom_>("zoom"_hs)
        .data<&Camera::nearPlane_>("nearPlane"_hs)
        .data<&Camera::farPlane_>("farPlane"_hs);
}
