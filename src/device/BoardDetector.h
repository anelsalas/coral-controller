#pragma once
#include "device/IPortEnumerator.h"

#include <optional>

namespace coral {

// Finds the serial port that belongs to the board, by USB vendor/product id (VIEW-001).
class BoardDetector {
public:
    explicit BoardDetector(IPortEnumerator& enumerator, UsbId target = kCoralDevBoardMicro);

    // ok + nullopt: enumeration worked but no board is attached.
    Result<std::optional<SerialPortInfo>> findBoard() const;

private:
    bool matchesTarget(const SerialPortInfo& port) const;

    IPortEnumerator& enumerator_;
    UsbId target_;
};

}  // namespace coral