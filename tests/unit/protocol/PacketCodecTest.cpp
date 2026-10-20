// PROTO-002: packet encoding (golden bytes) and round trip of every packet type.
#include "fakes/PacketBuilder.h"
#include "protocol/PacketDecoder.h"

#include <gtest/gtest.h>

using namespace coral::protocol;
using namespace coral::testing;

namespace {
struct CapturedPacket {
    PacketHeader header;
    std::vector<std::uint8_t> payload;
};

class CapturingListener final : public IPacketListener {
public:
    void onPacket(const PacketHeader& header, const std::uint8_t* payload, std::size_t length) override {
        packets.push_back({header, std::vector<std::uint8_t>(payload, payload + length)});
    }
    void onReject(RejectReason reason, const PacketHeader&) override { rejects.push_back(reason); }
    std::vector<CapturedPacket> packets;
    std::vector<RejectReason> rejects;
};
}  // namespace

TEST(PacketCodec, EncodesGoldenPacket) {
    const auto packet = encodePacket(PacketType::kLog, 1, bytesOf("hi"));

    // Generated independently with Python zlib.crc32 (see docs/PROTOCOL.md).
    const std::vector<std::uint8_t> golden = {0x43, 0x52, 0x4c, 0x50, 0x01, 0x03, 0x00, 0x00, 0x01, 0x00, 0x00,
                                              0x00, 0x02, 0x00, 0x00, 0x00, 0x68, 0x69, 0xdf, 0xb0, 0x7b, 0xb5};
    EXPECT_EQ(packet, golden);
}

TEST(PacketCodec, RoundTripsAllTypes) {
    std::vector<std::uint8_t> stream;
    append(stream, helloPacket("camera_stream_binary", "1.2.3", 0));
    append(stream, framePacket({0xFF, 0xD8, 0xFF, 0xD9}, 1, 324, 324));
    append(stream, encodePacket(PacketType::kLog, 2, bytesOf("boot ok")));
    std::vector<std::uint8_t> storage(kMaxPacketBytes);
    CapturingListener listener;
    PacketDecoder decoder(listener, storage.data(), storage.size());

    decoder.feed(stream.data(), stream.size());

    ASSERT_EQ(listener.packets.size(), 3u);
    HelloPayload hello;
    ASSERT_TRUE(decodeHello(listener.packets[0].payload.data(), listener.packets[0].payload.size(), &hello));
    EXPECT_STREQ(hello.appName, "camera_stream_binary");
    EXPECT_STREQ(hello.appVersion, "1.2.3");
    FrameInfo info;
    ASSERT_TRUE(decodeFrameInfo(listener.packets[1].payload.data(), listener.packets[1].payload.size(), &info));
    EXPECT_EQ(info.width, 324);
    EXPECT_EQ(listener.packets[1].payload.size(), kFrameInfoBytes + 4);
    EXPECT_EQ(listener.packets[2].header.type, PacketType::kLog);
    EXPECT_EQ(listener.packets[2].header.sequence, 2u);
    EXPECT_TRUE(listener.rejects.empty());
}

TEST(PacketCodec, DecodesByteByByte) {
    const auto packet = encodePacket(PacketType::kLog, 0, bytesOf("slow"));
    std::vector<std::uint8_t> storage(kMaxPacketBytes);
    CapturingListener listener;
    PacketDecoder decoder(listener, storage.data(), storage.size());

    for (const std::uint8_t byte : packet) decoder.feed(&byte, 1);

    ASSERT_EQ(listener.packets.size(), 1u);
    EXPECT_EQ(listener.packets[0].payload, bytesOf("slow"));
}

TEST(PacketDecoder, RejectsBadCrc) {
    auto packet = encodePacket(PacketType::kLog, 0, bytesOf("data"));
    packet[kHeaderBytes] ^= 0xFF;  // corrupt the payload
    std::vector<std::uint8_t> storage(kMaxPacketBytes);
    CapturingListener listener;
    PacketDecoder decoder(listener, storage.data(), storage.size());

    decoder.feed(packet.data(), packet.size());

    EXPECT_TRUE(listener.packets.empty());
    ASSERT_EQ(listener.rejects.size(), 1u);
    EXPECT_EQ(listener.rejects[0], RejectReason::kBadCrc);
    EXPECT_EQ(decoder.counters().badCrc, 1u);
}

TEST(PacketDecoder, RejectsOversizeLength) {
    auto packet = encodePacket(PacketType::kLog, 0, bytesOf("data"));
    putLe32(packet.data() + offsets::kPayloadLength, static_cast<std::uint32_t>(kMaxPayloadBytes + 1));
    std::vector<std::uint8_t> storage(kMaxPacketBytes);
    CapturingListener listener;
    PacketDecoder decoder(listener, storage.data(), storage.size());

    decoder.feed(packet.data(), packet.size());

    ASSERT_EQ(listener.rejects.size(), 1u);
    EXPECT_EQ(listener.rejects[0], RejectReason::kPayloadTooLarge);
    EXPECT_TRUE(listener.packets.empty());
}

TEST(PacketDecoder, RejectsUnknownVersion) {
    const auto packet = withVersion(encodePacket(PacketType::kLog, 0, bytesOf("v2")), kProtocolVersion + 1);
    std::vector<std::uint8_t> storage(kMaxPacketBytes);
    CapturingListener listener;
    PacketDecoder decoder(listener, storage.data(), storage.size());

    decoder.feed(packet.data(), packet.size());

    ASSERT_EQ(listener.rejects.size(), 1u);
    EXPECT_EQ(listener.rejects[0], RejectReason::kUnsupportedVersion);
    EXPECT_TRUE(listener.packets.empty());
}

TEST(PacketDecoder, ResyncsAfterGarbage) {
    std::vector<std::uint8_t> stream = bytesOf("boot noise CRL garbage\r\n");
    append(stream, encodePacket(PacketType::kLog, 0, bytesOf("first")));
    auto corrupt = encodePacket(PacketType::kLog, 1, bytesOf("corrupt"));
    corrupt[kHeaderBytes + 2] ^= 0x55;
    append(stream, corrupt);
    append(stream, encodePacket(PacketType::kLog, 2, bytesOf("second")));
    std::vector<std::uint8_t> storage(kMaxPacketBytes);
    CapturingListener listener;
    PacketDecoder decoder(listener, storage.data(), storage.size());

    decoder.feed(stream.data(), stream.size());

    ASSERT_EQ(listener.packets.size(), 2u);
    EXPECT_EQ(listener.packets[0].payload, bytesOf("first"));
    EXPECT_EQ(listener.packets[1].payload, bytesOf("second"));
    EXPECT_EQ(decoder.counters().badCrc, 1u);
    EXPECT_GT(decoder.counters().discardedBytes, 0u);
}