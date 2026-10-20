#pragma once
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace coral {

// Strict RFC 4648 base64 (standard alphabet, '=' padding, no whitespace).
std::optional<std::vector<std::uint8_t>> decodeBase64(std::string_view text);

}  // namespace coral