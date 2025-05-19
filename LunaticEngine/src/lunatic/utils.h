#pragma once

#include "pch.h"

namespace Lunatic::Utils {
    struct TransparentStringHash {
        using is_transparent = void;

        size_t operator()(const std::string& s) const noexcept;
        size_t operator()(std::string_view sv) const noexcept;
    };

    struct TransparentStringEqual {
        using is_transparent = void;

        bool operator()(const std::string& lhs, const std::string& rhs) const noexcept;
        bool operator()(const std::string& lhs, std::string_view rhs) const noexcept;
        bool operator()(std::string_view lhs, const std::string& rhs) const noexcept;
        bool operator()(std::string_view lhs, std::string_view rhs) const noexcept;
    };

    template<typename T>
    using unordered_string_map = std::unordered_map<std::string, T, TransparentStringHash, TransparentStringEqual>;
} // namespace Lunatic::Utils