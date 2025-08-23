// reflection.cpp
#include "pch.h"
#include "reflection.h"

#include <sol/sol.hpp>
#include <entt.hpp>

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <iostream>

namespace Lunatic {

void Lunatic::initReflection() {
    using namespace entt::literals;

    entt::meta_factory<Instance>()
        .type("Instance"_hs)
        .func<&Instance::getName>("getName"_hs)
        .func<&Instance::setName>("setName"_hs)
        .func<&Instance::getParent>("getParent"_hs)
        .func<&Instance::setParent>("setParent"_hs)
        .func<&Instance::getChildren>("getChildren"_hs)
        .func<&Instance::getClassName>("getClassName"_hs)
        .func<&Instance::findChildByName>("findChildByName"_hs)
        .func<&Instance::isA>("isA"_hs);

    entt::meta_factory<Renderable>()
        .type("Renderable"_hs)
        .func<&Renderable::setPosition>("setPosition"_hs)
        .func<&Renderable::getPosition>("getPosition"_hs)
        .func<&Renderable::setColor>("setColor"_hs)
        .func<&Renderable::getColor>("getColor"_hs);

    entt::meta_factory<Updateable>()
        .type("Updateable"_hs)
        .func<&Updateable::update>("update"_hs);

    entt::meta_factory<Sprite>()
        .type("Sprite"_hs)
        .base<Instance>()
        .base<Renderable>()
        .func<&Sprite::draw>("draw"_hs)
        .func<&Sprite::getVAO>("getVAO"_hs);

    entt::meta_factory<Script>()
        .type("Script"_hs)
        .base<Instance>()
        .base<Updateable>()
        .func<&Script::loadCode>("loadCode"_hs)
        .func<&Script::update>("update"_hs)
        .func<&Script::callCounter>("callCounter"_hs);

    entt::meta_factory<Camera>()
        .type("Camera"_hs)
        .base<Instance>()
        .func<&Camera::use>("use"_hs)
        .func<&Camera::resize>("resize"_hs)
        .func<&Camera::screenToWorld>("screenToWorld"_hs)
        .func<&Camera::getPosition>("getPosition"_hs)
        .func<&Camera::setPosition>("setPosition"_hs);
}

} // namespace Lunatic
