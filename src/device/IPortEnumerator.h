#pragma once
#include "common/Result.h"
#include "device/SerialPortInfo.h"

#include <vector>

namespace coral {

class IPortEnumerator {
public:
    virtual ~IPortEnumerator() = default;
    virtual Result<std::vector<SerialPortInfo>> listPorts() = 0;
};

}  // namespace coral