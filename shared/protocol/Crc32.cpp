#include "Crc32.h"

namespace coral {
namespace protocol {
namespace {

constexpr std::uint32_t kReflectedPolynomial = 0xEDB88320u;
constexpr std::uint32_t kFinalXor = 0xFFFFFFFFu;

struct CrcTable {
    std::uint32_t entries[256];
    constexpr CrcTable() : entries() {
        for (std::uint32_t byte = 0; byte < 256; ++byte) {
            std::uint32_t crc = byte;
            for (int bit = 0; bit < 8; ++bit) crc = (crc & 1u) ? (crc >> 1) ^ kReflectedPolynomial : crc >> 1;
            entries[byte] = crc;
        }
    }
};

constexpr CrcTable kTable{};  // computed at compile time, lives in flash on the board

}  // namespace

std::uint32_t crc32(const std::uint8_t* data, std::size_t length, std::uint32_t previousCrc) {
    std::uint32_t crc = previousCrc ^ kFinalXor;
    for (std::size_t i = 0; i < length; ++i) crc = kTable.entries[(crc ^ data[i]) & 0xFFu] ^ (crc >> 8);
    return crc ^ kFinalXor;
}

}  // namespace protocol
}  // namespace coral