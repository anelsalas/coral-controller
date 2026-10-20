#include "device/BoardDetector.h"

#include <algorithm>

namespace coral {

BoardDetector::BoardDetector(IPortEnumerator& enumerator, UsbId target) : enumerator_(enumerator), target_(target) {}

Result<std::optional<SerialPortInfo>> BoardDetector::findBoard() const {
    using DetectionResult = Result<std::optional<SerialPortInfo>>;
    const auto ports = enumerator_.listPorts();
    if (!ports.ok()) {
        return DetectionResult::failure("Cannot enumerate serial ports: " + ports.error());
    }
    const auto& list = ports.value();
    const auto match = std::find_if(list.begin(), list.end(), [this](const auto& port) { return matchesTarget(port); });
    if (match == list.end()) {
        return DetectionResult::success(std::nullopt);
    }
    return DetectionResult::success(*match);
}

bool BoardDetector::matchesTarget(const SerialPortInfo& port) const {
    return port.vendorId == target_.vendorId && port.productId == target_.productId;
}

}  // namespace coral