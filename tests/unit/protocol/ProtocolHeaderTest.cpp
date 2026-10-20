// PROTO-001: header layout is the documented wire format.
#include "protocol/CoralProtocol.h"

#include <gtest/gtest.h>

using namespace coral::protocol;

static_assert(kHeaderBytes == 16, "header size is part of the wire format");
static_assert(offsets::kPayloadLength + 4 == kHeaderBytes, "payload length is the last header field");
static_assert(kMaxPacketBytes == kHeaderBytes + kMaxPayloadBytes + kCrcBytes);

TEST(ProtocolHeader, LayoutMatchesSpec) {
    PacketHeader header;
    header.type = PacketType::kFrame;
    header.flags = 0x0102;
    header.sequence = 0x0A0B0C0D;
    header.payloadLength = 0x00010203;
    std::uint8_t bytes[kHeaderBytes] = {};

    writeHeader(header, bytes);

    const std::uint8_t expected[kHeaderBytes] = {'C', 'R', 'L', 'P', kProtocolVersion, 2, 0x02, 0x01,
                                                 0x0D, 0x0C, 0x0B, 0x0A, 0x03, 0x02, 0x01, 0x00};
    for (std::size_t i = 0; i < kHeaderBytes; ++i) EXPECT_EQ(bytes[i], expected[i]) << "offset " << i;
    PacketHeader decoded;
    ASSERT_TRUE(readHeader(bytes, &decoded));
    EXPECT_EQ(decoded.sequence, header.sequence);
    EXPECT_EQ(decoded.payloadLength, header.payloadLength);
    EXPECT_EQ(decoded.flags, header.flags);
}

TEST(ProtocolHeader, RejectsMissingMagic) {
    std::uint8_t bytes[kHeaderBytes] = {'X', 'R', 'L', 'P'};
    PacketHeader decoded;
    EXPECT_FALSE(readHeader(bytes, &decoded));
}