#pragma once
#include "device/ISerialPort.h"

#include <atomic>
#include <functional>
#include <span>
#include <thread>

namespace coral {

using ByteSink = std::function<void(std::span<const std::uint8_t>)>;
using ReadFailureSink = std::function<void(const std::string&)>;

// Reads an open port on a background thread (VIEW-002). Stops itself after a read failure
// and reports it once through ReadFailureSink. Callbacks run on the reader thread.
class SerialReader {
public:
    static constexpr std::chrono::milliseconds kReadTimeout{100};
    static constexpr std::size_t kReadChunkBytes = 4096;

    SerialReader(ISerialPort& port, ByteSink onData, ReadFailureSink onFailure);
    ~SerialReader();
    SerialReader(const SerialReader&) = delete;
    SerialReader& operator=(const SerialReader&) = delete;

    void start();
    void stop();
    bool isRunning() const;

private:
    void readLoop(std::stop_token stopToken);

    ISerialPort& port_;
    ByteSink onData_;
    ReadFailureSink onFailure_;
    std::atomic<bool> running_{false};
    std::jthread thread_;
};

}  // namespace coral