#include "pch.h"

#include "utils.h"

using namespace Lunatic::Utils;

size_t TransparentStringHash::operator()(const std::string& s) const noexcept {
    return std::hash<std::string>{}(s);
}

size_t TransparentStringHash::operator()(std::string_view sv) const noexcept {
    return std::hash<std::string_view>{}(sv);
}

bool TransparentStringEqual::operator()(const std::string& lhs, const std::string& rhs) const noexcept {
    return lhs == rhs;
}

bool TransparentStringEqual::operator()(const std::string& lhs, std::string_view rhs) const noexcept {
    return lhs == rhs;
}

bool TransparentStringEqual::operator()(std::string_view lhs, const std::string& rhs) const noexcept {
    return lhs == rhs;
}

bool TransparentStringEqual::operator()(std::string_view lhs, std::string_view rhs) const noexcept {
    return lhs == rhs;
}
