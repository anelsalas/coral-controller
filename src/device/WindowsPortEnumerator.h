#pragma once
#include "device/IPortEnumerator.h"

namespace coral {

// Lists present COM ports with their USB ids via SetupAPI.
class WindowsPortEnumerator final : public IPortEnumerator {
public:
    Result<std::vector<SerialPortInfo>> listPorts() override;
};

}  // namespace coral