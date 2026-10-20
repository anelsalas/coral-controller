#pragma once
#include "protocol/Crc32.h"
#include "protocol/PacketWriter.h"
#include "protocol/Payloads.h"

#include <string>
#include <vector>

namespace coral::testing {

class VectorSink final : public protocol::IByteSink {
public:
    bool write(const std::uint8_t* data, std::size_t length) override {
        if (failWrites) return false;
        bytes.insert(bytes.end(), data, data + length);
        return true;
    }
    std::vector<std::uint8_t> bytes;
    bool failWrites = false;
};

inline std::vector<std::uint8_t> encodePacket(protocol::PacketType type, std::uint32_t sequence,
                                              const std::vector<std::uint8_t>& payload) {
    VectorSink sink;
    protocol::PacketWriter writer(sink);
    for (std::uint32_t i = 0; i < sequence; ++i) sink.bytes.clear(), writer.write(type, nullptr, 0);
    sink.bytes.clear();
    writer.write(type, payload.data(), payload.size());
    return sink.bytes;
}

inline std::vector<std::uint8_t> bytesOf(const std::string& text) { return {text.begin(), text.end()}; }

inline std::vector<std::uint8_t> helloPacket(const char* name, const char* version, std::uint32_t sequence = 0) {
    std::vector<std::uint8_t> payload(protocol::kHelloPayloadBytes);
    protocol::encodeHello(name, version, payload.data());
    return encodePacket(protocol::PacketType::kHello, sequence, payload);
}

inline std::vector<std::uint8_t> framePacket(const std::vector<std::uint8_t>& jpeg, std::uint32_t sequence,
                                             std::uint16_t width = 324, std::uint16_t height = 324) {
    std::vector<std::uint8_t> payload(protocol::kFrameInfoBytes);
    protocol::encodeFrameInfo(protocol::FrameInfo{width, height, protocol::ImageFormat::kJpeg}, payload.data());
    payload.insert(payload.end(), jpeg.begin(), jpeg.end());
    return encodePacket(protocol::PacketType::kFrame, sequence, payload);
}

// Rewrites the version byte and re-seals the CRC, simulating firmware speaking another protocol version.
inline std::vector<std::uint8_t> withVersion(std::vector<std::uint8_t> packet, std::uint8_t version) {
    packet[protocol::offsets::kVersion] = version;
    const std::size_t crcOffset = packet.size() - protocol::kCrcBytes;
    protocol::putLe32(packet.data() + crcOffset, protocol::crc32(packet.data(), crcOffset));
    return packet;
}

inline void append(std::vector<std::uint8_t>& stream, const std::vector<std::uint8_t>& more) {
    stream.insert(stream.end(), more.begin(), more.end());
}

}  // namespace coral::testing