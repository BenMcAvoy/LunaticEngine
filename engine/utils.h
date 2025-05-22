#pragma once

#include <spdlog/spdlog.h>

#include <stdexcept>
#include <string>

namespace Lunatic {

class AssertionError : public std::runtime_error {
   public:
    AssertionError(const std::string& condition,
                   const std::string& file,
                   int line,
                   const std::string& message = "")
        : std::runtime_error(buildMessage(condition, file, line, message)) {}

   private:
    static std::string buildMessage(const std::string& cond,
                                    const std::string& file,
                                    int line,
                                    const std::string& msg) {
        if (!msg.empty()) {
            return "Assertion `" + cond + "` failed (" + msg + ") @ " + file + ":" + std::to_string(line);
        }
        return "Assertion `" + cond + "` failed @ " + file + ":" + std::to_string(line);
    }
};

}  // namespace Lunatic

inline void LUNA_ASSERT(bool condition, const char* condition_str, const char* file, int line, const char* message = nullptr) {
    if (!condition) {
        if (message) {
            spdlog::critical("Assertion `{}` failed. ({}) @ {}:{}", condition_str, message, file, line);
        } else {
            spdlog::critical("Assertion `{}` failed. @ {}:{}", condition_str, file, line);
        }
        std::abort();
    }
}

inline void lun_throw_assert(bool condition, const char* condition_str, const char* file, int line, const char* message = nullptr) {
    if (!condition) {
        if (message) {
            spdlog::error("Assertion `{}` failed. ({}) @ {}:{}", condition_str, message, file, line);
        } else {
            spdlog::error("Assertion `{}` failed. @ {}:{}", condition_str, file, line);
        }
        throw Lunatic::AssertionError(condition_str, file, line, message ? message : "");
    }
}

// Macros
#define LUNA_ASSERT(cond, ...) \
    LUNA_ASSERT((cond), #cond, __FILE__, __LINE__, ##__VA_ARGS__)

#define LUNA_THROW_ASSERT(cond, ...) \
    lun_throw_assert((cond), #cond, __FILE__, __LINE__, ##__VA_ARGS__)

#define LUNADEBUG(...) \
    SPDLOG_DEBUG(__VA_ARGS__)
#define LUNAINFO(...) \
    SPDLOG_INFO(__VA_ARGS__)
#define LUNAWARN(...) \
    SPDLOG_WARN(__VA_ARGS__)
#define LUNAERROR(...) \
    SPDLOG_ERROR(__VA_ARGS__)
#define LUNACRITICAL(...) \
    SPDLOG_CRITICAL(__VA_ARGS__)