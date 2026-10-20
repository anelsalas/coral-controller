#pragma once
#include "common/Clock.h"
#include "device/BoardDetector.h"
#include "device/ISerialPort.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <span>
#include <stop_token>
#include <string>
#include <thread>

namespace coral {

inline constexpr unsigned kDefaultBaudRate = 115200;  // current camera_streaming_serial firmware

using ByteSink = std::function<void(std::span<const std::uint8_t>)>;

struct ConnectionSettings {
    unsigned baudRate = kDefaultBaudRate;
    std::chrono::milliseconds retryInterval{1000};
    std::chrono::milliseconds shutdownTimeout{2000};
};

enum class ConnectionState { Searching, Connected };

// Listener callbacks and ByteSink run on the connection worker thread, in order:
// onConnected, data..., onDisconnected.
class IConnectionListener {
public:
    virtual ~IConnectionListener() = default;
    virtual void onConnected(const SerialPortInfo& port) = 0;
    virtual void onDisconnected(const std::string& reason) = 0;
    virtual void onConnectError(const std::string& message) = 0;
};

// Finds, opens, reads and closes the board's serial port on one worker thread and reconnects
// after loss (VIEW-001, VIEW-002, VIEW-012). No call on the caller's (UI) thread touches the
// device, so a wedged USB driver cannot freeze the UI.
class ConnectionManager {
public:
    static constexpr std::chrono::milliseconds kReadTimeout{100};
    static constexpr std::size_t kReadChunkBytes = 4096;

    ConnectionManager(IPortEnumerator& enumerator, ISerialPort& port, const IClock& clock, ByteSink onData,
                      IConnectionListener& listener, ConnectionSettings settings = {});
    ~ConnectionManager();
    ConnectionManager(const ConnectionManager&) = delete;
    ConnectionManager& operator=(const ConnectionManager&) = delete;

    void start();
    // Waits at most settings.shutdownTimeout. Returns false if the worker was stuck in a device
    // call and had to be abandoned: it then makes no further callbacks, but the port object must
    // stay alive until the process exits.
    bool shutdown();
    ConnectionState state() const;

private:
    struct WorkerControl {
        std::atomic<bool> abandoned{false};
        std::mutex mutex;
        std::condition_variable finishedChanged;
        bool finished = false;
    };

    void run(std::stop_token stop, const WorkerControl& control);
    void connectAndStream(std::stop_token stop, const WorkerControl& control);
    std::string streamUntilFailureOrStop(std::stop_token stop, const WorkerControl& control);
    void waitUntil(TimePoint deadline, std::stop_token stop) const;

    BoardDetector detector_;
    ISerialPort& port_;
    const IClock& clock_;
    ByteSink onData_;
    IConnectionListener& listener_;
    ConnectionSettings settings_;

    std::atomic<ConnectionState> state_{ConnectionState::Searching};
    std::shared_ptr<WorkerControl> control_;
    std::jthread worker_;
};

}  // namespace coral