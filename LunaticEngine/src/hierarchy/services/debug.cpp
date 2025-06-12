#include "pch.h"
#include "debug.h"
#include "../../core/engine.h"
#include <spdlog/spdlog.h>
#include <fmt/format.h>

using namespace Lunatic::Services;

// ImGuiConsole Implementation
ImGuiConsole::ImGuiConsole() : auto_scroll_(true), scroll_to_bottom_(false) {
	Clear();
}

void ImGuiConsole::Clear() {
	logs_.clear();
	filter_.Clear();
}

void ImGuiConsole::AddLog(const ImVec4& col, const char* fmt, ...) {
	char buf[1024];
	va_list args;
	va_start(args, fmt);
	std::vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	logs_.push_back({ std::string(buf), col });
	if (auto_scroll_) {
		scroll_to_bottom_ = true;
	}
}

void ImGuiConsole::Draw(const char* title, bool* p_open) {
	if (!ImGui::Begin(title, p_open)) {
		ImGui::End();
		return;
	}

	// Controls
	if (ImGui::Button("Options")) ImGui::OpenPopup("ConsoleOptions");
	ImGui::SameLine();
	if (ImGui::Button("Clear")) Clear();
	ImGui::SameLine();
	if (ImGui::Button("Copy")) ImGui::LogToClipboard();
	ImGui::SameLine();
	filter_.Draw("Filter (inc,-exc)");

	if (ImGui::BeginPopup("ConsoleOptions")) {
		ImGui::Checkbox("Auto-scroll", &auto_scroll_);
		ImGui::EndPopup();
	}

	ImGui::Separator();

	// Main scrolling region
	ImGui::BeginChild(
		"ScrollingRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()),
		false, ImGuiWindowFlags_HorizontalScrollbar);

	for (const auto& entry : logs_) {
		if (!filter_.IsActive() || filter_.PassFilter(entry.message.c_str())) {
			ImGui::PushStyleColor(ImGuiCol_Text, entry.color);
			ImGui::TextUnformatted(entry.message.c_str());
			ImGui::PopStyleColor();
		}
	}

	if (scroll_to_bottom_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
		ImGui::SetScrollHereY(1.0f);
	}
	scroll_to_bottom_ = false;

	ImGui::EndChild();
	ImGui::Separator();

	ImGui::End();
}

CustomSink::CustomSink(ImGuiConsole* console) : console_(console) {}

void CustomSink::sink_it_(const spdlog::details::log_msg& msg) {
	spdlog::memory_buf_t formatted;
	formatter_->format(msg, formatted);
	std::string formattedMessage = fmt::to_string(formatted);
	auto color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // default white
	switch (msg.level) {
		// gray (trace)
	case spdlog::level::trace:
		color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
		break;
		// green (info)
	case spdlog::level::info:
		color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
		break;
		// yellow (warn)
	case spdlog::level::warn:
		color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
		break;
		// red (err/critical)
	case spdlog::level::err:
	case spdlog::level::critical:
		color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
		break;
	}

	console_->AddLog(color, "%s", formattedMessage.c_str());
}

void CustomSink::flush_() {
}

Debug::Debug() : Service("Debug") {
	// Set up spdlog with our custom sink
	m_customSink = std::make_shared<CustomSink>(&m_console);
	spdlog::sinks_init_list sinks = { m_customSink };
	spdlog::set_default_logger(std::make_shared<spdlog::logger>("Lunatic", sinks));
	spdlog::set_level(spdlog::level::trace);
}

void Debug::update(float deltaTime) {
	auto& engine = Engine::GetInstance();
	for (const auto& [name, service] : engine.getServicesMap()) {
		if (m_autoUpdateMap[name]) {
			service->update(deltaTime);
		}
	}
}

void Debug::render() {
	if (m_showServices) {
		renderServicesWindow();
	}
	
	if (m_showConsole) {
		renderConsoleWindow();
	}
	if (m_showScripting) {
		static auto scripting = ServiceLocator::Get<Lunatic::Services::Scripting>("Scripting");
		scripting->drawImGuiWindow();
	}

	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("Debug")) {
			ImGui::MenuItem("Services", nullptr, &m_showServices);
			ImGui::MenuItem("Console", nullptr, &m_showConsole);
			ImGui::MenuItem("Scripting", nullptr, &m_showScripting);
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}

void Debug::renderServicesWindow() {
	if (!ImGui::Begin("Lunatic Services", &m_showServices)) {
		ImGui::End();
		return;
	}

	auto& engine = Engine::GetInstance();

	if (ImGui::BeginTable("ServicesTable", 3, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoBordersInBody)) {
		// Table headers
		ImGui::TableSetupColumn("Service");
		ImGui::TableSetupColumn("Auto Update");
		ImGui::TableSetupColumn("Auto Render");
		ImGui::TableHeadersRow();

		for (const auto& [name, service] : engine.getServicesMap()) {
			// If it's `Debug` service, skip it to avoid toggling itself
			if (name == "Debug") {
				continue;
			}

			ImGui::TableNextRow();
			ImGui::PushID(name.data());

			m_autoUpdateMap.try_emplace(name, false);
			m_autoRenderMap.try_emplace(name, false);

			ImGui::TableSetColumnIndex(0);
			ImGui::Text("%s", name.data());

			ImGui::TableSetColumnIndex(1);
			ImGui::Checkbox("##AutoUpdate", &m_autoUpdateMap[name]);

			ImGui::TableSetColumnIndex(2);
			ImGui::Checkbox("##AutoRender", &m_autoRenderMap[name]);

			if (m_autoRenderMap[name]) {
				auto renderableService = std::dynamic_pointer_cast<IRenderable>(service);
				if (renderableService) {
					renderableService->render();
				}
			}

			ImGui::PopID();
		}
		ImGui::EndTable();
	}

	ImGui::End();
}

void Debug::renderConsoleWindow() {
	m_console.Draw("Console", &m_showConsole);
}
