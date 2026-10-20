// VIEW-013: status reflects connection and stream events.
#include "status/StatusModel.h"

#include <gtest/gtest.h>

using namespace coral;
using namespace std::chrono_literals;

TEST(StatusModel, ReflectsConnectionEvents) {
    StatusModel status;
    EXPECT_EQ(status.snapshot(TimePoint{}).link, LinkState::Searching);

    status.onConnected(SerialPortInfo{"COM8", 0x18D1, 0x9308});
    auto connected = status.snapshot(TimePoint{});
    EXPECT_EQ(connected.link, LinkState::Connected);
    EXPECT_EQ(connected.portName, "COM8");
    EXPECT_EQ(connected.protocol, "text (FRAME:base64)");
    EXPECT_EQ(connected.boardApp, "unknown");

    status.onDisconnected("device unplugged");
    auto lost = status.snapshot(TimePoint{});
    EXPECT_EQ(lost.link, LinkState::Searching);
    EXPECT_EQ(lost.lastError, "device unplugged");
}

TEST(StatusModel, CountsFramesAndErrors) {
    StatusModel status;
    const TimePoint start{};
    status.onFrameDecoded(start);
    status.onFrameDecoded(start + 500ms);
    status.onParseError(ParseErrorKind::InvalidBase64, "bad");
    status.onParseError(ParseErrorKind::OverlongLine, "long");
    status.onDecodeError("corrupt");
    status.onLogLine("Camera serial streaming!");

    const auto snapshot = status.snapshot(start + 500ms);

    EXPECT_EQ(snapshot.framesDecoded, 2u);
    EXPECT_NEAR(snapshot.framesPerSecond, 2.0, 1e-9);
    EXPECT_EQ(snapshot.invalidBase64Frames, 1u);
    EXPECT_EQ(snapshot.overlongLines, 1u);
    EXPECT_EQ(snapshot.jpegDecodeErrors, 1u);
    ASSERT_EQ(snapshot.recentLogLines.size(), 1u);
    EXPECT_EQ(snapshot.recentLogLines.back(), "Camera serial streaming!");
}