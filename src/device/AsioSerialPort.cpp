#include "device/AsioSerialPort.h"

#include <boost/asio/buffer.hpp>

#include <optional>

namespace coral {
namespace {

constexpr unsigned kDataBits = 8;
// "\\.\" prefix is required by Windows for COM10 and above; harmless below.
constexpr const char* kWin32DevicePrefix = "\\\\.\\";

std::string describe(const std::string& action, const boost::system::error_code& error) {
    return action + ": " + error.message() + " (code " + std::to_string(error.value()) + ")";
}

}  // namespace

Status AsioSerialPort::open(const std::string& portName, unsigned baudRate) {
    boost::system::error_code error;
    port_.open(kWin32DevicePrefix + portName, error);
    if (error) return Status::failure(describe("Cannot open " + portName, error));
    const Status configured = configure(baudRate);
    if (!configured.ok()) {
        const Status closed = close();
        return Status::failure(configured.error() + (closed.ok() ? "" : "; " + closed.error()));
    }
    return okStatus();
}

Status AsioSerialPort::configure(unsigned baudRate) {
    using boost::asio::serial_port_base;
    boost::system::error_code error;
    port_.set_option(serial_port_base::baud_rate(baudRate), error);
    if (error) return Status::failure(describe("Cannot set baud rate", error));
    port_.set_option(serial_port_base::character_size(kDataBits), error);
    if (error) return Status::failure(describe("Cannot set data bits", error));
    port_.set_option(serial_port_base::parity(serial_port_base::parity::none), error);
    if (error) return Status::failure(describe("Cannot set parity", error));
    port_.set_option(serial_port_base::stop_bits(serial_port_base::stop_bits::one), error);
    if (error) return Status::failure(describe("Cannot set stop bits", error));
    port_.set_option(serial_port_base::flow_control(serial_port_base::flow_control::none), error);
    if (error) return Status::failure(describe("Cannot set flow control", error));
    return okStatus();
}

Status AsioSerialPort::close() {
    if (!port_.is_open()) return okStatus();
    boost::system::error_code error;
    port_.close(error);
    if (error) return Status::failure(describe("Cannot close serial port", error));
    return okStatus();
}

bool AsioSerialPort::isOpen() const { return port_.is_open(); }

ReadResult AsioSerialPort::read(std::span<std::uint8_t> buffer, std::chrono::milliseconds timeout) {
    std::optional<boost::system::error_code> completion;
    std::size_t bytesRead = 0;
    port_.async_read_some(boost::asio::buffer(buffer.data(), buffer.size()),
                          [&](const boost::system::error_code& error, std::size_t count) {
                              completion = error;
                              bytesRead = count;
                          });
    io_.restart();
    io_.run_for(timeout);
    if (!completion) {
        boost::system::error_code cancelError;
        port_.cancel(cancelError);
        io_.restart();
        io_.run();  // let the cancelled handler complete before the buffer goes away
        if (cancelError) return ReadResult{ReadStatus::Failed, 0, describe("Cannot cancel read", cancelError)};
    }
    if (bytesRead > 0) return ReadResult{ReadStatus::Data, bytesRead, {}};
    if (!completion || *completion == boost::asio::error::operation_aborted) {
        return ReadResult{ReadStatus::Timeout, 0, {}};
    }
    if (*completion) return ReadResult{ReadStatus::Failed, 0, describe("Serial read failed", *completion)};
    return ReadResult{ReadStatus::Timeout, 0, {}};
}

}  // namespace coral