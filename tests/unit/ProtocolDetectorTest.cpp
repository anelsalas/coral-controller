// PROTO-009: automatic text/binary protocol detection.
#include "fakes/PacketBuilder.h"
#include "protocol/ProtocolDetector.h"

#include <gtest/gtest.h>

using namespace coral;
using namespace coral::testing;

TEST(ProtocolDetector, SelectsTextForFrameLines) {
    ProtocolDetector detector;
    const auto stream = bytesOf("Camera serial streaming!\r\nFRAME:AQID\r\n");

    EXPECT_EQ(detector.feed(stream), StreamProtocol::Text);
    EXPECT_EQ(detector.takeBufferedBytes(), stream) << "earlier log lines are replayed to the text parser";
}

TEST(ProtocolDetector, SelectsBinaryForMagic) {
    ProtocolDetector detector;
    std::vector<std::uint8_t> stream = bytesOf("boot text\r\n");
    const auto packet = helloPacket("app", "1.0");
    append(stream, packet);

    EXPECT_EQ(detector.feed(stream), StreamProtocol::Binary);
    EXPECT_EQ(detector.takeBufferedBytes(), packet) << "replay starts at the magic";
}

TEST(ProtocolDetector, WaitsUntilDecisionIsPossible) {
    ProtocolDetector detector;
    EXPECT_EQ(detector.feed(bytesOf("FRA")), StreamProtocol::Unknown);
    EXPECT_EQ(detector.feed(bytesOf("ME:AQID\r\n")), StreamProtocol::Text);
    EXPECT_EQ(detector.takeBufferedBytes(), bytesOf("FRAME:AQID\r\n"));
}

TEST(ProtocolDetector, FallsBackToTextAfterProbeLimit) {
    ProtocolDetector detector;
    const std::vector<std::uint8_t> noise(ProtocolDetector::kMaxProbeBytes + 1, 'x');
    EXPECT_EQ(detector.feed(noise), StreamProtocol::Text);
}