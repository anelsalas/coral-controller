// PROTO-008: firmware-side encoder (host build of the shared code).
#include "fakes/PacketBuilder.h"

#include <gtest/gtest.h>

using namespace coral::protocol;
using coral::testing::VectorSink;

TEST(FirmwarePacketEncoder, FailsOnOversizePayload) {
    VectorSink sink;
    PacketWriter writer(sink);
    std::vector<std::uint8_t> payload(kMaxPayloadBytes + 1);

    EXPECT_EQ(writer.write(PacketType::kFrame, payload.data(), payload.size()), WriteStatus::kPayloadTooLarge);
    EXPECT_TRUE(sink.bytes.empty()) << "nothing is sent for a rejected payload";
    EXPECT_EQ(writer.nextSequence(), 0u);
}

TEST(FirmwarePacketEncoder, ReportsSinkFailureAndConsumesSequence) {
    VectorSink sink;
    sink.failWrites = true;
    PacketWriter writer(sink);
    const std::uint8_t byte = 1;

    EXPECT_EQ(writer.write(PacketType::kLog, &byte, 1), WriteStatus::kSinkFailed);
    EXPECT_EQ(writer.nextSequence(), 1u);
}

TEST(FirmwarePacketEncoder, SplitPayloadEqualsContiguousPayload) {
    VectorSink split;
    VectorSink contiguous;
    const std::uint8_t first[] = {1, 2, 3};
    const std::uint8_t second[] = {4, 5};
    const std::uint8_t all[] = {1, 2, 3, 4, 5};

    PacketWriter(split).write(PacketType::kFrame, first, 3, second, 2);
    PacketWriter(contiguous).write(PacketType::kFrame, all, 5);

    EXPECT_EQ(split.bytes, contiguous.bytes);
}