// Hardware tests: need a Coral Dev Board Micro running camera_streaming_serial.
// Run with: ctest --preset arm64-release-hardware
#include "common/Clock.h"
#include "device/AsioSerialPort.h"
#include "device/ConnectionManager.h"
#include "device/WindowsPortEnumerator.h"
#include "filters/BuiltinFilters.h"
#include "filters/DefaultPipeline.h"
#include "imaging/LatestFrameBuffer.h"
#include "imaging/SnapshotWriter.h"
#include "status/StatusModel.h"
#include "viewer/ViewerSession.h"

#include <gtest/gtest.h>

#include <cstdlib>
#include <iostream>
#include <thread>

using namespace coral;
using namespace std::chrono_literals;

namespace {
constexpr auto kFirstFrameTimeout = 20s;
constexpr auto kReplugTimeout = 60s;
constexpr int kBoardImageSize = 324;

struct LiveViewer {
    SteadyClock clock;
    WindowsPortEnumerator enumerator;
    AsioSerialPort port;
    StatusModel status;
    LatestFrameBuffer frames;
    ViewerSession session{frames, status, clock};
    ConnectionManager connection{enumerator, port, clock,
                                 [this](std::span<const std::uint8_t> bytes) { session.onBytes(bytes); },
                                 session};

    template <typename Predicate>
    bool runUntil(Predicate predicate, std::chrono::seconds limit) {
        const auto deadline = std::chrono::steady_clock::now() + limit;
        while (!predicate()) {
            if (std::chrono::steady_clock::now() > deadline) return false;
            connection.tick();
            std::this_thread::sleep_for(10ms);
        }
        return true;
    }
};
}  // namespace

TEST(Hardware, ViewerShowsUprightImage) {
    LiveViewer viewer;
    std::optional<Frame> frame;

    const bool gotFrame = viewer.runUntil([&] { return (frame = viewer.frames.takeLatest()).has_value(); },
                                          kFirstFrameTimeout);
    viewer.connection.shutdown();

    ASSERT_TRUE(gotFrame) << "no frame within timeout; last error: "
                          << viewer.status.snapshot(viewer.clock.now()).lastError;
    EXPECT_EQ(frame->image.cols, kBoardImageSize);
    EXPECT_EQ(frame->image.rows, kBoardImageSize);

    FilterRegistry registry;
    registerBuiltinFilters(registry);
    const auto output = makeDefaultPipeline(registry).apply(frame->image);
    ASSERT_FALSE(output.error.has_value()) << output.error->message;
    const auto saved = SnapshotWriter("snapshots").write(output.image, std::chrono::system_clock::now(), "hardware");
    ASSERT_TRUE(saved.ok()) << saved.error();
    std::cout << "Snapshot for manual upright check: " << saved.value().string() << "\n";
}

TEST(Hardware, ReconnectsAfterReplug) {
    std::size_t valueLength = 0;
    getenv_s(&valueLength, nullptr, 0, "CORAL_MANUAL_REPLUG");
    if (valueLength == 0) {
        GTEST_SKIP() << "set CORAL_MANUAL_REPLUG=1 and unplug/replug the board during the test";
    }
    LiveViewer viewer;
    auto isConnected = [&] { return viewer.connection.state() == ConnectionState::Connected; };
    ASSERT_TRUE(viewer.runUntil(isConnected, kFirstFrameTimeout));
    std::cout << "Unplug the board now, then plug it back in.\n";
    ASSERT_TRUE(viewer.runUntil([&] { return !isConnected(); }, kReplugTimeout));
    ASSERT_TRUE(viewer.runUntil(isConnected, kReplugTimeout));
    ASSERT_TRUE(viewer.runUntil([&] { return viewer.frames.takeLatest().has_value(); }, kFirstFrameTimeout));
    viewer.connection.shutdown();
}