#include "CoralProtocol.h"

#include <cstring>

namespace coral {
namespace protocol {

void putLe16(std::uint8_t* out, std::uint16_t value) {
    out[0] = static_cast<std::uint8_t>(value & 0xFFu);
    out[1] = static_cast<std::uint8_t>((value >> 8) & 0xFFu);
}

void putLe32(std::uint8_t* out, std::uint32_t value) {
    for (int i = 0; i < 4; ++i) out[i] = static_cast<std::uint8_t>((value >> (8 * i)) & 0xFFu);
}

std::uint16_t getLe16(const std::uint8_t* in) {
    return static_cast<std::uint16_t>(in[0] | (in[1] << 8));
}

std::uint32_t getLe32(const std::uint8_t* in) {
    return static_cast<std::uint32_t>(in[0]) | (static_cast<std::uint32_t>(in[1]) << 8) |
           (static_cast<std::uint32_t>(in[2]) << 16) | (static_cast<std::uint32_t>(in[3]) << 24);
}

void writeHeader(const PacketHeader& header, std::uint8_t* out) {
    std::memcpy(out + offsets::kMagic, kMagic, kMagicBytes);
    out[offsets::kVersion] = header.version;
    out[offsets::kType] = static_cast<std::uint8_t>(header.type);
    putLe16(out + offsets::kFlags, header.flags);
    putLe32(out + offsets::kSequence, header.sequence);
    putLe32(out + offsets::kPayloadLength, header.payloadLength);
}

bool readHeader(const std::uint8_t* in, PacketHeader* header) {
    if (std::memcmp(in + offsets::kMagic, kMagic, kMagicBytes) != 0) return false;
    header->version = in[offsets::kVersion];
    header->type = static_cast<PacketType>(in[offsets::kType]);
    header->flags = getLe16(in + offsets::kFlags);
    header->sequence = getLe32(in + offsets::kSequence);
    header->payloadLength = getLe32(in + offsets::kPayloadLength);
    return true;
}

}  // namespace protocol
}  // namespace coral