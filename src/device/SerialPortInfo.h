#pragma once
#include <cstdint>
#include <string>

namespace coral {

struct UsbId {
    std::uint16_t vendorId;
    std::uint16_t productId;
};

inline constexpr UsbId kCoralDevBoardMicro{0x18D1, 0x9308};

struct SerialPortInfo {
    std::string portName;  // e.g. "COM8"
    std::uint16_t vendorId = 0;
    std::uint16_t productId = 0;
};

}  // namespace coral