// PROTO-006, PROTO-010 and the PC side of PROTO-007: binary stream through the viewer session.
#include "fakes/FakeClock.h"
#include "fakes/PacketBuilder.h"
#include "fakes/TestImages.h"
#include "viewer/ViewerSession.h"

#include <gtest/gtest.h>

using namespace coral;
using namespace coral::testing;

namespace {
class BinaryViewerSessionTest : public ::testing::Test {
protected:
    void SetUp() override { session.onConnected(SerialPortInfo{"COM8", 0x18D1, 0x9308}); }
    void feed(const std::vector<std::uint8_t>& bytes) { session.onBytes(bytes); }
    StatusSnapshot status() { return statusModel.snapshot(clock.now()); }

    FakeClock clock;
    StatusModel statusModel;
    LatestFrameBuffer frames;
    ViewerSession session{frames, statusModel, clock};
};
}  // namespace

TEST_F(BinaryViewerSessionTest, HelloPacketUpdatesBoardIdentity) {
    feed(helloPacket("camera_stream_binary", "2.0.0"));

    EXPECT_EQ(status().boardApp, "camera_stream_binary 2.0.0");
    EXPECT_EQ(status().protocol, StatusModel::kBinaryProtocolName);
}

TEST_F(BinaryViewerSessionTest, FramePacketReachesDisplay) {
    feed(helloPacket("app", "1"));
    feed(framePacket(boardSizedJpeg(), 1));

    const auto frame = frames.takeLatest();
    ASSERT_TRUE(frame.has_value());
    EXPECT_EQ(frame->image.cols, kBoardImageSize);
    EXPECT_EQ(status().framesDecoded, 1u);
}

TEST_F(BinaryViewerSessionTest, ReportsDroppedPackets) {
    feed(helloPacket("app", "1", 0));
    feed(framePacket(boardSizedJpeg(), 1));
    feed(framePacket(boardSizedJpeg(), 4));

    EXPECT_EQ(status().droppedPackets, 2u);
}

TEST_F(BinaryViewerSessionTest, VersionCheckFlagsIncompatibleMajorVersion) {
    feed(withVersion(helloPacket("future_app", "9.0"), protocol::kProtocolVersion + 1));

    EXPECT_NE(status().boardApp.find("incompatible"), std::string::npos) << status().boardApp;
    EXPECT_EQ(status().rejectedPackets, 1u);
}

TEST_F(BinaryViewerSessionTest, TextProtocolStillWorks) {
    feed(bytesOf(frameLine(boardSizedJpeg())));

    EXPECT_TRUE(frames.takeLatest().has_value());
    EXPECT_EQ(status().protocol, StatusModel::kTextProtocolName);
    EXPECT_EQ(status().boardApp, StatusModel::kUnknownBoardApp);
}

TEST_F(BinaryViewerSessionTest, ReconnectRedetectsProtocol) {
    feed(bytesOf(frameLine(boardSizedJpeg())));
    session.onDisconnected("unplugged");
    session.onConnected(SerialPortInfo{"COM8", 0x18D1, 0x9308});

    feed(helloPacket("camera_stream_binary", "1.0.0"));

    EXPECT_EQ(status().protocol, StatusModel::kBinaryProtocolName);
}