#include "protocol/Base64.h"

namespace coral {
namespace {

constexpr std::size_t kCharsPerGroup = 4;
constexpr std::size_t kBytesPerGroup = 3;
constexpr char kPadding = '=';
constexpr int kInvalid = -1;

int sextetOf(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return kInvalid;
}

std::uint8_t byte(int value) { return static_cast<std::uint8_t>(value & 0xFF); }

}  // namespace

std::optional<std::vector<std::uint8_t>> decodeBase64(std::string_view text) {
    if (text.size() % kCharsPerGroup != 0) return std::nullopt;
    std::vector<std::uint8_t> bytes;
    bytes.reserve(text.size() / kCharsPerGroup * kBytesPerGroup);
    for (std::size_t i = 0; i < text.size(); i += kCharsPerGroup) {
        const bool isLastGroup = i + kCharsPerGroup == text.size();
        const int s0 = sextetOf(text[i]);
        const int s1 = sextetOf(text[i + 1]);
        if (s0 == kInvalid || s1 == kInvalid) return std::nullopt;
        bytes.push_back(byte((s0 << 2) | (s1 >> 4)));

        if (isLastGroup && text[i + 2] == kPadding) {
            if (text[i + 3] != kPadding) return std::nullopt;
            break;
        }
        const int s2 = sextetOf(text[i + 2]);
        if (s2 == kInvalid) return std::nullopt;
        bytes.push_back(byte((s1 << 4) | (s2 >> 2)));

        if (isLastGroup && text[i + 3] == kPadding) break;
        const int s3 = sextetOf(text[i + 3]);
        if (s3 == kInvalid) return std::nullopt;
        bytes.push_back(byte((s2 << 6) | s3));
    }
    return bytes;
}

}  // namespace coral