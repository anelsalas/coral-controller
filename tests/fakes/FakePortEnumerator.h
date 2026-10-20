#pragma once
#include "device/IPortEnumerator.h"

#include <mutex>

namespace coral::testing {

class FakePortEnumerator final : public IPortEnumerator {
public:
    void setPorts(std::vector<SerialPortInfo> ports) {
        std::lock_guard lock(mutex_);
        ports_ = std::move(ports);
    }

    Result<std::vector<SerialPortInfo>> listPorts() override {
        std::lock_guard lock(mutex_);
        return Result<std::vector<SerialPortInfo>>::success(ports_);
    }

private:
    std::mutex mutex_;
    std::vector<SerialPortInfo> ports_;
};

inline SerialPortInfo boardPort(std::string name) {
    return SerialPortInfo{std::move(name), kCoralDevBoardMicro.vendorId, kCoralDevBoardMicro.productId};
}

}  // namespace coral::testing