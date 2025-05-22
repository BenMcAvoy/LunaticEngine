#include <imgui.h>
#include <spdlog/details/log_msg.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/spdlog.h>

#include <mutex>

#include "utils.h"

namespace Lunatic::Debug {
struct Log {
    std::string message;
    std::string file;
    int line;
    std::string function;
    ImVec4 color;
    std::chrono::system_clock::time_point time;
};

class Console {
   public:
    Console() = default;
    ~Console() = default;

    static Console& GetInstance() {
        static Console s_instance;
        return s_instance;
    }

    void addLog(const Log& log);

    void renderWindow() {
        std::scoped_lock lock(mutex);
        ImGui::Begin("Console");
        for (const auto& log : logs) {
            ImGui::TextColored(log.color, "%s", log.message.c_str());
            if (ImGui::BeginItemTooltip()) {
                ImGui::Text("File: %s", log.file.c_str());
                ImGui::Text("Line: %d", log.line);
                ImGui::Text("Function: %s", log.function.c_str());
                if (log.time != std::chrono::system_clock::time_point{}) {
                    auto time = std::chrono::system_clock::to_time_t(log.time);
                    // ImGui::Text("Time: %s", std::ctime(&time));

                    auto res = std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
                    ImGui::Text("Time: %s", res);
                }
                if (ImGui::Button("Copy")) {
                    ImGui::SetClipboardText(log.message.c_str());
                }
                ImGui::EndTooltip();
            }
        }
        ImGui::End();
    }

    std::vector<Log> logs;
    std::mutex mutex;
};

template <typename Mutex>
class ConsoleSink : public spdlog::sinks::base_sink<Mutex> {
   protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        auto& log = Console::GetInstance();

        spdlog::memory_buf_t formatted;
        this->formatter_->format(msg, formatted);
        auto str = formatted.data();

        Log logEntry;
        logEntry.message = std::string(str, str + formatted.size());
        logEntry.file = msg.source.filename;
        logEntry.line = msg.source.line;
        logEntry.function = msg.source.funcname;
        logEntry.time = msg.time;
        logEntry.color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);  // Default color
        switch (msg.level) {
            case spdlog::level::trace:
                logEntry.color = ImVec4(0.5f, 0.5f, 1.0f, 1.0f);
                break;
            case spdlog::level::debug:
                logEntry.color = ImVec4(0.5f, 0.5f, 1.0f, 1.0f);
                break;
            case spdlog::level::info:
                logEntry.color = ImVec4(0.5f, 1.0f, 0.5f, 1.0f);
                break;
            case spdlog::level::warn:
                logEntry.color = ImVec4(1.0f, 1.0f, 0.5f, 1.0f);
                break;
            case spdlog::level::err:
                logEntry.color = ImVec4(1.0f, 0.5f, 0.5f, 1.0f);
                break;
            case spdlog::level::critical:
                logEntry.color = ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
                break;
            default:
                break;
        }
        log.addLog(logEntry);
    }
    void flush_() override { /* No need to flush a GUI console */ }
};

static void SetupConsole() {
    auto consoleSink = std::make_shared<ConsoleSink<std::mutex>>();
    auto consoleLogger = std::make_shared<spdlog::logger>("console", consoleSink);
    spdlog::register_logger(consoleLogger);
    spdlog::set_default_logger(consoleLogger);
    spdlog::set_level(spdlog::level::trace);
}
}  // namespace Lunatic::Debug