#pragma once
#include "common/Clock.h"
#include "device/BoardDetector.h"
#include "device/SerialReader.h"

#include <memory>
#include <mutex>
#include <optional>

namespace coral {

inline constexpr unsigned kDefaultBaudRate = 115200;  // current camera_streaming_serial firmware

struct ConnectionSettings {
    unsigned baudRate = kDefaultBaudRate;
    std::chrono::milliseconds retryInterval{1000};
};

enum class ConnectionState { Searching, Connected };

class IConnectionListener {
public:
    virtual ~IConnectionListener() = default;
    virtual void onConnected(const SerialPortInfo& port) = 0;
    virtual void onDisconnected(const std::string& reason) = 0;
    virtual void onConnectError(const std::string& message) = 0;
};

// Finds, opens and watches the board's serial port; reconnects after loss (VIEW-001, VIEW-012).
// tick() is called regularly from one thread (the UI loop); data arrives on the reader thread.
class ConnectionManager {
public:
    ConnectionManager(IPortEnumerator& enumerator, ISerialPort& port, const IClock& clock, ByteSink onData,
                      IConnectionListener& listener, ConnectionSettings settings = {});
    ~ConnectionManager();
    ConnectionManager(const ConnectionManager&) = delete;
    ConnectionManager& operator=(const ConnectionManager&) = delete;

    void tick();
    void shutdown();
    ConnectionState state() const;

private:
    void tryConnect();
    void handleReadFailureIfAny();
    void disconnect(const std::string& reason);
    void recordReadFailure(const std::string& message);

    BoardDetector detector_;
    ISerialPort& port_;
    const IClock& clock_;
    ByteSink onData_;
    IConnectionListener& listener_;
    ConnectionSettings settings_;

    ConnectionState state_ = ConnectionState::Searching;
    TimePoint nextAttempt_{};
    std::unique_ptr<SerialReader> reader_;
    std::mutex failureMutex_;
    std::optional<std::string> pendingReadFailure_;
};

}  // namespace coral