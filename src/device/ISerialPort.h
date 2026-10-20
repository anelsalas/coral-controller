#pragma once
#include "common/Result.h"

#include <chrono>
#include <cstdint>
#include <span>
#include <string>

namespace coral {

enum class ReadStatus { Data, Timeout, Failed };

struct ReadResult {
    ReadStatus status = ReadStatus::Timeout;
    std::size_t bytesRead = 0;
    std::string error;
};

// Serial link abstraction; real implementation uses Boost.Asio, tests use a fake.
class ISerialPort {
public:
    virtual ~ISerialPort() = default;
    virtual Status open(const std::string& portName, unsigned baudRate) = 0;
    virtual Status close() = 0;
    virtual bool isOpen() const = 0;
    // Waits at most `timeout` for data.
    virtual ReadResult read(std::span<std::uint8_t> buffer, std::chrono::milliseconds timeout) = 0;
};

}  // namespace coral