// Coral Controller binary protocol v1 - shared by board firmware and PC (PROTO-001/002).
// Embedded-friendly: C++17, no exceptions, no RTTI needed, no heap.
//
// Packet (all multi-byte fields little-endian):
//   offset  size  field
//   0       4     magic "CRLP"
//   4       1     protocol version (kProtocolVersion)
//   5       1     packet type (PacketType)
//   6       2     flags (reserved, 0)
//   8       4     sequence number (per board boot, starts at 0, wraps)
//   12      4     payload length (<= kMaxPayloadBytes)
//   16      n     payload
//   16+n    4     CRC-32 (IEEE 802.3, as zlib/Boost crc_32_type) over bytes [0, 16+n)
#pragma once

#include <cstddef>
#include <cstdint>

namespace coral {
namespace protocol {

constexpr std::uint8_t kMagic[4] = {'C', 'R', 'L', 'P'};
constexpr std::size_t kMagicBytes = sizeof(kMagic);
constexpr std::uint8_t kProtocolVersion = 1;
constexpr std::size_t kHeaderBytes = 16;
constexpr std::size_t kCrcBytes = 4;
constexpr std::size_t kMaxPayloadBytes = 128 * 1024;
constexpr std::size_t kMaxPacketBytes = kHeaderBytes + kMaxPayloadBytes + kCrcBytes;

namespace offsets {
constexpr std::size_t kMagic = 0;
constexpr std::size_t kVersion = 4;
constexpr std::size_t kType = 5;
constexpr std::size_t kFlags = 6;
constexpr std::size_t kSequence = 8;
constexpr std::size_t kPayloadLength = 12;
}  // namespace offsets

enum class PacketType : std::uint8_t {
    kHello = 1,  // payload: HelloPayload (app name + app version)
    kFrame = 2,  // payload: FrameInfo + encoded image
    kLog = 3,    // payload: UTF-8 text, no terminator
};

struct PacketHeader {
    std::uint8_t version = kProtocolVersion;
    PacketType type = PacketType::kLog;
    std::uint16_t flags = 0;
    std::uint32_t sequence = 0;
    std::uint32_t payloadLength = 0;
};

void writeHeader(const PacketHeader& header, std::uint8_t* out /* kHeaderBytes */);
// False if the bytes do not start with kMagic. Does not validate version/length.
bool readHeader(const std::uint8_t* in /* kHeaderBytes */, PacketHeader* header);

void putLe16(std::uint8_t* out, std::uint16_t value);
void putLe32(std::uint8_t* out, std::uint32_t value);
std::uint16_t getLe16(const std::uint8_t* in);
std::uint32_t getLe32(const std::uint8_t* in);

}  // namespace protocol
}  // namespace coral