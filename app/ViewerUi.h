#pragma once
#include "FrameTexture.h"
#include "status/StatusModel.h"
#include "ui/FilterPanelModel.h"

#include <string>

namespace coral::app {

struct UiRequests {
    bool saveSnapshot = false;
};

// Dear ImGui views. Filter controls are generated from FilterParameter descriptors (VIEW-008).
class ViewerUi {
public:
    UiRequests draw(const FrameTexture& frame, FilterPanelModel& filters, const StatusSnapshot& status,
                    const std::string& pipelineError, const std::string& lastSnapshotMessage);

private:
    void drawCameraWindow(const FrameTexture& frame, UiRequests& requests, const std::string& lastSnapshotMessage);
    void drawFilterPanel(FilterPanelModel& filters, const std::string& pipelineError);
    void drawFilterEntry(FilterPanelModel& filters, std::size_t filterIndex);
    void drawParameterControl(FilterPanelModel& filters, std::size_t filterIndex, std::size_t parameterIndex);
    void drawStatusWindow(const StatusSnapshot& status);
    void drawLogWindow(const StatusSnapshot& status);
};

}  // namespace coral::app