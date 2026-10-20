#pragma once
#include "device/SerialPortInfo.h"

#include <optional>
#include <string_view>

namespace coral {

// Parses a Windows hardware id such as "USB\VID_18D1&PID_9308&MI_00".
std::optional<UsbId> parseUsbHardwareId(std::string_view hardwareId);

}  // namespace coral