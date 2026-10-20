#include "ViewerApp.h"

#include "Dx11Window.h"
#include "FrameTexture.h"
#include "ViewerUi.h"
#include "device/AsioSerialPort.h"
#include "device/ConnectionManager.h"
#include "device/WindowsPortEnumerator.h"
#include "filters/BuiltinFilters.h"
#include "filters/DefaultPipeline.h"
#include "imaging/LatestFrameBuffer.h"
#include "imaging/SnapshotWriter.h"
#include "viewer/ViewerSession.h"

#include <iostream>

namespace coral::app {
namespace {
constexpr int kInitialWidth = 1100;
constexpr int kInitialHeight = 920;
constexpr int kExitOk = 0;
constexpr int kExitStartupFailed = 1;
constexpr int kExitSmokeTimeout = 2;
constexpr int kExitSnapshotFailed = 3;
constexpr auto kSmokeTimeout = std::chrono::seconds(60);

class Viewer {
public:
    Viewer(const AppOptions& options, Dx11Window& window)
        : options_(options),
          window_(window),
          snapshots_(options.snapshotFolder),
          pipeline_(makeDefaultPipeline(registryWithBuiltins())),
          panel_(pipeline_),
          texture_(window.device(), window.context()) {}

    int run() {
        const TimePoint started = clock_.now();
        while (window_.pumpMessages()) {
            connection_.tick();
            showNewestFrame();
            window_.beginFrame();
            const UiRequests requests =
                ui_.draw(texture_, panel_, status_.snapshot(clock_.now()), pipelineError_, snapshotMessage_);
            if (const auto presented = window_.endFrame(); !presented.ok()) status_.onError(presented.error());
            if (requests.saveSnapshot) saveSnapshots();
            if (options_.snapshotAfterFrames) {
                if (displayedFrames_ >= *options_.snapshotAfterFrames) return finishSmokeRun();
                if (clock_.now() - started > kSmokeTimeout) return failSmokeRun();
            }
        }
        return kExitOk;
    }

private:
    static FilterRegistry registryWithBuiltins() {
        FilterRegistry registry;
        registerBuiltinFilters(registry);
        return registry;
    }

    void showNewestFrame() {
        auto frame = frames_.takeLatest();
        if (!frame) return;
        PipelineOutput output = pipeline_.apply(frame->image);
        pipelineError_ = output.error ? output.error->filterName + ": " + output.error->message : "";
        if (const auto uploaded = texture_.upload(output.image); !uploaded.ok()) {
            status_.onError(uploaded.error());
            return;
        }
        rawFrame_ = frame->image;
        processedFrame_ = output.image;
        ++displayedFrames_;
    }

    bool saveSnapshots() {
        const auto now = std::chrono::system_clock::now();
        const auto processed = snapshots_.write(processedFrame_, now, "processed");
        const auto raw = snapshots_.write(rawFrame_, now, "raw");
        if (!processed.ok() || !raw.ok()) {
            snapshotMessage_ = "Snapshot failed: " + (processed.ok() ? raw.error() : processed.error());
            status_.onError(snapshotMessage_);
            return false;
        }
        snapshotMessage_ = "Saved " + processed.value().string();
        std::cout << snapshotMessage_ << "\n" << "Saved " << raw.value().string() << std::endl;
        return true;
    }

    int finishSmokeRun() {
        std::cout << "Smoke run: displayed " << displayedFrames_ << " frames" << std::endl;
        connection_.shutdown();
        return saveSnapshots() ? kExitOk : kExitSnapshotFailed;
    }

    int failSmokeRun() {
        std::cerr << "Smoke run: timeout, displayed " << displayedFrames_
                  << " frames; last error: " << status_.snapshot(clock_.now()).lastError << std::endl;
        connection_.shutdown();
        return kExitSmokeTimeout;
    }

    const AppOptions& options_;
    Dx11Window& window_;
    SteadyClock clock_;
    WindowsPortEnumerator enumerator_;
    AsioSerialPort port_;
    StatusModel status_;
    LatestFrameBuffer frames_;
    ViewerSession session_{frames_, status_, clock_};
    ConnectionManager connection_{enumerator_, port_, clock_,
                                  [this](std::span<const std::uint8_t> bytes) { session_.onBytes(bytes); }, status_};
    SnapshotWriter snapshots_;
    FilterPipeline pipeline_;
    FilterPanelModel panel_;
    FrameTexture texture_;
    ViewerUi ui_;
    cv::Mat rawFrame_;
    cv::Mat processedFrame_;
    std::string pipelineError_;
    std::string snapshotMessage_;
    int displayedFrames_ = 0;
};

}  // namespace

int runViewerApp(const AppOptions& options) {
    auto window = Dx11Window::create(L"Coral Controller", kInitialWidth, kInitialHeight);
    if (!window.ok()) {
        std::cerr << "Cannot create window: " << window.error() << std::endl;
        return kExitStartupFailed;
    }
    Viewer viewer(options, *window.value());
    return viewer.run();
}

}  // namespace coral::app