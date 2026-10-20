#include "Payloads.h"

#include "CoralProtocol.h"

#include <cstring>

namespace coral {
namespace protocol {
namespace {

void copyPadded(const char* text, std::uint8_t* out, std::size_t fieldBytes) {
    std::memset(out, 0, fieldBytes);
    if (text == nullptr) return;
    for (std::size_t i = 0; i < fieldBytes && text[i] != '\0'; ++i) out[i] = static_cast<std::uint8_t>(text[i]);
}

void copyField(const std::uint8_t* in, std::size_t fieldBytes, char* out) {
    std::memcpy(out, in, fieldBytes);
    out[fieldBytes] = '\0';
}

constexpr std::size_t kWidthOffset = 0;
constexpr std::size_t kHeightOffset = 2;
constexpr std::size_t kFormatOffset = 4;

}  // namespace

std::size_t encodeHello(const char* appName, const char* appVersion, std::uint8_t* out) {
    copyPadded(appName, out, kAppNameBytes);
    copyPadded(appVersion, out + kAppNameBytes, kAppVersionBytes);
    return kHelloPayloadBytes;
}

bool decodeHello(const std::uint8_t* payload, std::size_t length, HelloPayload* hello) {
    if (length != kHelloPayloadBytes) return false;
    copyField(payload, kAppNameBytes, hello->appName);
    copyField(payload + kAppNameBytes, kAppVersionBytes, hello->appVersion);
    return true;
}

void encodeFrameInfo(const FrameInfo& info, std::uint8_t* out) {
    std::memset(out, 0, kFrameInfoBytes);
    putLe16(out + kWidthOffset, info.width);
    putLe16(out + kHeightOffset, info.height);
    out[kFormatOffset] = static_cast<std::uint8_t>(info.format);
}

bool decodeFrameInfo(const std::uint8_t* payload, std::size_t length, FrameInfo* info) {
    if (length < kFrameInfoBytes) return false;
    info->width = getLe16(payload + kWidthOffset);
    info->height = getLe16(payload + kHeightOffset);
    info->format = static_cast<ImageFormat>(payload[kFormatOffset]);
    return info->format == ImageFormat::kJpeg;
}

}  // namespace protocol
}  // namespace coral