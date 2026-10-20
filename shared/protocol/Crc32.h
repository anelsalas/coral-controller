#pragma once
#include <cstddef>
#include <cstdint>

namespace coral {
namespace protocol {

// CRC-32/ISO-HDLC (zlib, Boost crc_32_type): reflected poly 0xEDB88320, init/xorout 0xFFFFFFFF.
// Incremental: crc32(b, n2, crc32(a, n1)) == crc32(a ++ b).
std::uint32_t crc32(const std::uint8_t* data, std::size_t length, std::uint32_t previousCrc = 0);

}  // namespace protocol
}  // namespace coral