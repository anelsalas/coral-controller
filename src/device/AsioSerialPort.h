#pragma once
#include "device/ISerialPort.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/serial_port.hpp>

namespace coral {

// 8N1, no flow control. Reads use async_read_some + run_for to get a real timeout.
class AsioSerialPort final : public ISerialPort {
public:
    Status open(const std::string& portName, unsigned baudRate) override;
    Status close() override;
    bool isOpen() const override;
    ReadResult read(std::span<std::uint8_t> buffer, std::chrono::milliseconds timeout) override;

private:
    Status configure(unsigned baudRate);

    boost::asio::io_context io_;
    boost::asio::serial_port port_{io_};
};

}  // namespace coral