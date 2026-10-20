#include "device/SerialReader.h"

#include <stdexcept>
#include <vector>

namespace coral {

SerialReader::SerialReader(ISerialPort& port, ByteSink onData, ReadFailureSink onFailure)
    : port_(port), onData_(std::move(onData)), onFailure_(std::move(onFailure)) {}

SerialReader::~SerialReader() { stop(); }

void SerialReader::start() {
    if (thread_.joinable()) throw std::logic_error("SerialReader::start called twice");
    running_ = true;
    thread_ = std::jthread([this](std::stop_token stopToken) { readLoop(stopToken); });
}

void SerialReader::stop() {
    if (!thread_.joinable()) return;
    thread_.request_stop();
    thread_.join();
}

bool SerialReader::isRunning() const { return running_; }

void SerialReader::readLoop(std::stop_token stopToken) {
    std::vector<std::uint8_t> buffer(kReadChunkBytes);
    while (!stopToken.stop_requested()) {
        const ReadResult result = port_.read(buffer, kReadTimeout);
        if (result.status == ReadStatus::Data) {
            onData_(std::span<const std::uint8_t>(buffer.data(), result.bytesRead));
        } else if (result.status == ReadStatus::Failed) {
            running_ = false;
            onFailure_(result.error);
            return;
        }
    }
    running_ = false;
}

}  // namespace coral