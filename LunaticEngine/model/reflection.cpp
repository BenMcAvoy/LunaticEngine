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

entt::meta_any luaToENTT(sol::object obj) {
    auto type = obj.get_type();
	std::println("[reflection] luaToENTT: type = {}", (int)type);
    switch (type) {
        case sol::type::number:
            return entt::meta_any(obj.as<double>());
        case sol::type::string:
            return entt::meta_any(obj.as<std::string>());
        case sol::type::boolean:
            return entt::meta_any(obj.as<bool>());
        case sol::type::table: {
            // Handle table conversion if needed
            return entt::meta_any(obj.as<sol::table>());
        }
        default:
            return entt::meta_any();
	}
}

sol::object enttToLua(sol::state_view ts, const entt::meta_any& any) {
	//std::println("[reflection] enttToLua: type = {}", any.type().info().name());

    if (!any) {
	    std::println("[reflection] enttToLua: any is null");
        return sol::make_object(ts, sol::nil);
    }

    //int
    if (any.type() == entt::resolve<int>()) {
        return sol::make_object(ts, any.cast<int>());
    } 
    // glm::vec2
    if (any.type() == entt::resolve<glm::vec2>()) {
        return sol::make_object(ts, any.cast<glm::vec2>());
	}
    else if (any.type() == entt::resolve<double>()) {
        return sol::make_object(ts, any.cast<double>());
    } 
    else if (any.type() == entt::resolve<std::string>()) {
        return sol::make_object(ts, any.cast<std::string>());
    }
    else if (any.type() == entt::resolve<std::string_view>()) {
		return sol::make_object(ts, std::string(any.cast<std::string_view>()));
    }
    else if (any.type() == entt::resolve<bool>()) {
        return sol::make_object(ts, any.cast<bool>());
	}
    // support std::shared_ptr<T>
    else if (any.type().info().name() == entt::resolve<std::shared_ptr<Instance>>().info().name()) {
        auto sp = any.cast<std::shared_ptr<Instance>>();
		return sol::make_object(ts, sp);
    }
    else if (any.type().info().name() == entt::resolve<std::shared_ptr<Script>>().info().name()) {
        auto sp = any.cast<std::shared_ptr<Script>>();
        return sol::make_object(ts, sp);
    }
    else {
		std::println("[reflection] enttToLua: unsupported type for conversion");
        return sol::make_object(ts, any);
    }
}

template<typename T>
entt::meta_any callMethod(std::shared_ptr<Instance> instance, std::string_view methodName, std::vector<entt::meta_any> args) {
	entt::meta_type& mt = instance->metaType;

    entt::meta_func mf = mt.func(entt::hashed_string{ methodName.data() });
    if (!mf) {
        std::println("[reflection] callMethod: method '{}' not found in type '{}'", methodName, mt.info().name());
        return {};
    }

    // Invoke: construct handle inline to avoid copy/move issues.
	T& tRef = static_cast<T&>(*instance);
	entt::meta_any result;
    if (args.empty()) {
        result = mf.invoke(tRef);
    } else {
        result = mf.invoke(tRef, args.data(), args.size());
    }
    if (!result) {
        std::println("[reflection] callMethod: invocation failed for method '{}' on type '{}'", methodName, mt.info().name());
        return {};
    }
	return result;
}

void Lunatic::initReflection() {
    using namespace entt::literals;

    entt::meta_factory<Instance>()
        .type("Instance"_hs)
        .func<&Instance::getName>("getName"_hs)
        .func<&Instance::setName>("setName"_hs)
        .func<&Instance::getParent>("getParent"_hs)
        .func<&Instance::setParent>("setParent"_hs)
        .func<&Instance::getChildren>("getChildren"_hs)
        .func<&Instance::getClassName>("getClassName"_hs);

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
}

} // namespace Lunatic
