#include "ViewerUi.h"

#include <imgui.h>

#include <algorithm>
#include <vector>

namespace coral::app {
namespace {
const ImVec4 kConnectedColor(0.3f, 0.9f, 0.4f, 1.0f);
const ImVec4 kSearchingColor(0.95f, 0.7f, 0.2f, 1.0f);
const ImVec4 kErrorColor(1.0f, 0.4f, 0.4f, 1.0f);

ImVec2 fitInside(ImVec2 image, ImVec2 available) {
    if (image.x <= 0 || image.y <= 0) return image;
    const float scale = std::max(0.1f, std::min(available.x / image.x, available.y / image.y));
    return ImVec2(image.x * scale, image.y * scale);
}
}  // namespace

UiRequests ViewerUi::draw(const FrameTexture& frame, FilterPanelModel& filters, const StatusSnapshot& status,
                          const std::string& pipelineError, const std::string& lastSnapshotMessage) {
    UiRequests requests;
    drawCameraWindow(frame, requests, lastSnapshotMessage);
    drawFilterPanel(filters, pipelineError);
    drawStatusWindow(status);
    drawLogWindow(status);
    return requests;
}

void ViewerUi::drawCameraWindow(const FrameTexture& frame, UiRequests& requests,
                                const std::string& lastSnapshotMessage) {
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(680, 720), ImGuiCond_FirstUseEver);
    ImGui::Begin("Camera");
    if (ImGui::Button("Snapshot")) requests.saveSnapshot = true;
    ImGui::SameLine();
    ImGui::TextUnformatted(lastSnapshotMessage.c_str());
    if (frame.hasImage()) {
        ImGui::Image(frame.textureId(), fitInside(frame.size(), ImGui::GetContentRegionAvail()));
    } else {
        ImGui::TextUnformatted("Waiting for frames...");
    }
    ImGui::End();
}

void ViewerUi::drawFilterPanel(FilterPanelModel& filters, const std::string& pipelineError) {
    ImGui::SetNextWindowPos(ImVec2(700, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(380, 480), ImGuiCond_FirstUseEver);
    ImGui::Begin("Filters");
    if (!pipelineError.empty()) ImGui::TextColored(kErrorColor, "%s", pipelineError.c_str());
    for (std::size_t i = 0; i < filters.filterCount(); ++i) drawFilterEntry(filters, i);
    ImGui::End();
}

void ViewerUi::drawFilterEntry(FilterPanelModel& filters, std::size_t filterIndex) {
    ImGui::PushID(static_cast<int>(filterIndex));
    bool enabled = filters.isEnabled(filterIndex);
    if (ImGui::Checkbox("##enabled", &enabled)) filters.setEnabled(filterIndex, enabled);
    ImGui::SameLine();
    if (ImGui::ArrowButton("##up", ImGuiDir_Up)) filters.moveUp(filterIndex);
    ImGui::SameLine();
    if (ImGui::ArrowButton("##down", ImGuiDir_Down)) filters.moveDown(filterIndex);
    ImGui::SameLine();
    const std::string name(filters.filterName(filterIndex));
    if (ImGui::TreeNode(name.c_str())) {
        for (std::size_t p = 0; p < filters.parameters(filterIndex).size(); ++p) {
            drawParameterControl(filters, filterIndex, p);
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

void ViewerUi::drawParameterControl(FilterPanelModel& filters, std::size_t filterIndex, std::size_t parameterIndex) {
    const FilterParameter& parameter = filters.parameters(filterIndex)[parameterIndex];
    const char* label = parameter.name().c_str();
    switch (parameter.kind()) {
        case ParameterKind::Integer: {
            int value = parameter.asInt();
            if (ImGui::SliderInt(label, &value, static_cast<int>(parameter.minimum()),
                                 static_cast<int>(parameter.maximum()))) {
                filters.setParameter(filterIndex, parameterIndex, value);
            }
            break;
        }
        case ParameterKind::Real: {
            float value = static_cast<float>(parameter.asReal());
            if (ImGui::SliderFloat(label, &value, static_cast<float>(parameter.minimum()),
                                   static_cast<float>(parameter.maximum()))) {
                filters.setParameter(filterIndex, parameterIndex, value);
            }
            break;
        }
        case ParameterKind::Boolean: {
            bool value = parameter.asBool();
            if (ImGui::Checkbox(label, &value)) filters.setParameter(filterIndex, parameterIndex, value ? 1 : 0);
            break;
        }
        case ParameterKind::Choice: {
            std::vector<const char*> items;
            for (const auto& option : parameter.options()) items.push_back(option.c_str());
            int selected = parameter.asChoiceIndex();
            if (ImGui::Combo(label, &selected, items.data(), static_cast<int>(items.size()))) {
                filters.setParameter(filterIndex, parameterIndex, selected);
            }
            break;
        }
    }
}

void ViewerUi::drawStatusWindow(const StatusSnapshot& status) {
    ImGui::SetNextWindowPos(ImVec2(700, 500), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(380, 230), ImGuiCond_FirstUseEver);
    ImGui::Begin("Status");
    if (status.link == LinkState::Connected) {
        ImGui::TextColored(kConnectedColor, "Connected on %s", status.portName.c_str());
    } else {
        ImGui::TextColored(kSearchingColor, "Searching for Coral Dev Board Micro (USB 18D1:9308)...");
    }
    ImGui::Text("Board app: %s", status.boardApp.empty() ? "-" : status.boardApp.c_str());
    ImGui::Text("Protocol:  %s", status.protocol.empty() ? "-" : status.protocol.c_str());
    ImGui::Text("Frames: %llu   fps: %.1f", static_cast<unsigned long long>(status.framesDecoded),
                status.framesPerSecond);
    ImGui::Text("Errors: base64 %llu, overlong %llu, jpeg %llu",
                static_cast<unsigned long long>(status.invalidBase64Frames),
                static_cast<unsigned long long>(status.overlongLines),
                static_cast<unsigned long long>(status.jpegDecodeErrors));
    ImGui::Text("Packets: rejected %llu, dropped %llu", static_cast<unsigned long long>(status.rejectedPackets),
                static_cast<unsigned long long>(status.droppedPackets));
    if (!status.lastError.empty()) ImGui::TextColored(kErrorColor, "Last: %s", status.lastError.c_str());
    ImGui::End();
}

void ViewerUi::drawLogWindow(const StatusSnapshot& status) {
    ImGui::SetNextWindowPos(ImVec2(10, 740), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(1070, 150), ImGuiCond_FirstUseEver);
    ImGui::Begin("Board log");
    for (const auto& line : status.recentLogLines) ImGui::TextUnformatted(line.c_str());
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
    ImGui::End();
}

}  // namespace coral::app