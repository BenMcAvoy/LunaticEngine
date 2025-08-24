#pragma once

#include "pch.h"

namespace Lunatic::Utils {
	struct StringHash {
		using is_transparent = void;

		size_t operator()(std::string_view str) const noexcept {
			return std::hash<std::string_view>{}(str);
		}

		size_t operator()(const std::string& str) const noexcept {
			return std::hash<std::string_view>{}(str);
		}

		size_t operator()(const char* str) const noexcept {
			return std::hash<std::string_view>{}(str);
		}
	};

	struct StringEqual {
		using is_transparent = void;

		bool operator()(std::string_view lhs, std::string_view rhs) const noexcept {
			return lhs == rhs;
		}
		bool operator()(const std::string& lhs, const std::string& rhs) const noexcept {
			return lhs == rhs;
		}
		bool operator()(const char* lhs, const char* rhs) const noexcept {
			return std::string_view(lhs) == std::string_view(rhs);
		}
	};

	template <typename T>
	using HeteroStringMap = std::unordered_map<std::string, T, StringHash, StringEqual>;
} // namespace Lunatic::Utils