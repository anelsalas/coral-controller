#include "device/UsbHardwareId.h"

#include <charconv>

namespace coral {
namespace {

constexpr std::string_view kUsbPrefix = "USB\\";
constexpr std::string_view kVendorTag = "VID_";
constexpr std::string_view kProductTag = "PID_";
constexpr std::size_t kHexIdDigits = 4;
constexpr int kHexBase = 16;

std::optional<std::uint16_t> hexIdAfterTag(std::string_view text, std::string_view tag) {
    const auto position = text.find(tag);
    if (position == std::string_view::npos) return std::nullopt;
    const auto digits = text.substr(position + tag.size(), kHexIdDigits);
    if (digits.size() != kHexIdDigits) return std::nullopt;
    std::uint16_t value = 0;
    const auto [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), value, kHexBase);
    if (error != std::errc{} || end != digits.data() + digits.size()) return std::nullopt;
    return value;
}

}  // namespace

std::optional<UsbId> parseUsbHardwareId(std::string_view hardwareId) {
    if (!hardwareId.starts_with(kUsbPrefix)) return std::nullopt;
    const auto vendor = hexIdAfterTag(hardwareId, kVendorTag);
    const auto product = hexIdAfterTag(hardwareId, kProductTag);
    if (!vendor || !product) return std::nullopt;
    return UsbId{*vendor, *product};
}

}  // namespace coral