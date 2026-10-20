#pragma once
#include <cstddef>
#include <cstdint>

namespace coral {
namespace protocol {

// HELLO payload: fixed-size, zero-padded ASCII fields (PROTO-006).
constexpr std::size_t kAppNameBytes = 32;
constexpr std::size_t kAppVersionBytes = 16;
constexpr std::size_t kHelloPayloadBytes = kAppNameBytes + kAppVersionBytes;

struct HelloPayload {
    char appName[kAppNameBytes + 1] = {};        // always NUL-terminated after decode
    char appVersion[kAppVersionBytes + 1] = {};
};

// Truncates longer strings. Returns bytes written (kHelloPayloadBytes).
std::size_t encodeHello(const char* appName, const char* appVersion, std::uint8_t* out /* kHelloPayloadBytes */);
bool decodeHello(const std::uint8_t* payload, std::size_t length, HelloPayload* hello);

// FRAME payload prefix, followed by the encoded image bytes.
constexpr std::size_t kFrameInfoBytes = 8;
enum class ImageFormat : std::uint8_t { kJpeg = 1 };

struct FrameInfo {
    std::uint16_t width = 0;
    std::uint16_t height = 0;
    ImageFormat format = ImageFormat::kJpeg;
};

void encodeFrameInfo(const FrameInfo& info, std::uint8_t* out /* kFrameInfoBytes */);
bool decodeFrameInfo(const std::uint8_t* payload, std::size_t length, FrameInfo* info);

}  // namespace protocol
}  // namespace coral