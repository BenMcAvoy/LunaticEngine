#pragma once

#include "../base.h"
#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/log_msg.h>
#include <imgui.h>
#include <unordered_map>

namespace Lunatic::Services {
	struct LogContainer {
		std::string message;
		ImVec4 color;
	};

	class ImGuiConsole {
	public:
		ImGuiConsole();
		void Clear();
		void AddLog(const ImVec4& col, const char* fmt, ...) IM_FMTARGS(3);
		void Draw(const char* title, bool* p_open = nullptr);

	private:
		std::vector<LogContainer> logs_;
		ImGuiTextFilter filter_;
		bool auto_scroll_;
		bool scroll_to_bottom_;
	};

	class CustomSink : public spdlog::sinks::base_sink<std::mutex> {
	public:
		CustomSink(ImGuiConsole* console);

	protected:
		void sink_it_(const spdlog::details::log_msg& msg) override;
		void flush_() override;

	private:
		ImGuiConsole* console_;
	};

	class Debug : public Service, public IRenderable {
	public:
		Debug();

		void update(float deltaTime) override;
		void render() override;

		// Console access
		ImGuiConsole& getConsole() { return m_console; }

	private:
		void renderServicesWindow();
		void renderConsoleWindow();

		ImGuiConsole m_console;
		std::shared_ptr<CustomSink> m_customSink;
		
		// Service debug controls
		std::unordered_map<std::string, bool> m_autoUpdateMap;
		std::unordered_map<std::string, bool> m_autoRenderMap;
		
		// Window visibility flags
		bool m_showServices = true;
		bool m_showConsole = true;
		bool m_showScripting = true;
	};
} // namespace Lunatic::Services
