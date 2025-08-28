#include "pch.h"

#include "primitives/script.h"
#include "primitives/sprite.h"
#include "primitives/camera.h"
#include "primitives/nativescript.h"

extern "C" void _force_link_anchor() {}

RTTR_REGISTRATION{
rttr::registration::class_<Lunatic::Instance>("Instance")
.property("name", &Lunatic::Instance::getName, &Lunatic::Instance::setName)
.property("parent", &Lunatic::Instance::getParent, &Lunatic::Instance::setParent)
.property_readonly("children", &Lunatic::Instance::getChildren)
.property_readonly("className", &Lunatic::Instance::getClassName)
.method("findChildByName", &Lunatic::Instance::findChildByName)
.method("getChildren", &Lunatic::Instance::getChildren)
.method("isA", &Lunatic::Instance::isA);

rttr::registration::class_<Lunatic::Renderable>("Renderable")
.property("position", &Lunatic::Renderable::getPosition, &Lunatic::Renderable::setPosition)
.property("scale", &Lunatic::Renderable::getScale, &Lunatic::Renderable::setScale)
.property("color", &Lunatic::Renderable::getColor, &Lunatic::Renderable::setColor);

rttr::registration::class_<Lunatic::Updateable>("Updateable"); // Nothing should be reflected here

rttr::registration::class_<Lunatic::Sprite>("Sprite")
.property("texturePath", &Lunatic::Sprite::getTexturePath, &Lunatic::Sprite::setTexture)
.method("clearTexture", &Lunatic::Sprite::clearTexture);

rttr::registration::class_<Lunatic::Script>("Script")
.property_readonly("codePath", &Lunatic::Script::codePath_);

rttr::registration::class_<Lunatic::NativeScript>("NativeScript"); // Nothing should be reflected here

rttr::registration::class_<Lunatic::Camera>("Camera")
.property_readonly("viewportSize", &Lunatic::Camera::viewportSize_)
.property("position", &Lunatic::Camera::getPosition, &Lunatic::Camera::setPosition)
.property("rotation", &Lunatic::Camera::rotation_)
.property_readonly("zoom", &Lunatic::Camera::zoom_)
.property_readonly("nearPlane", &Lunatic::Camera::nearPlane_)
.property_readonly("farPlane", &Lunatic::Camera::farPlane_)
.method("screenToWorld", &Lunatic::Camera::screenToWorld)
.method("worldToScreen", &Lunatic::Camera::worldToScreen);
}

/*
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
*/
