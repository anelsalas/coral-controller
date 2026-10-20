#include "device/ConnectionManager.h"

#include <vector>

namespace coral {
namespace {
constexpr auto kIdlePoll = std::chrono::milliseconds(5);
constexpr char kShutdownReason[] = "application shutdown";
}  // namespace

ConnectionManager::ConnectionManager(IPortEnumerator& enumerator, ISerialPort& port, const IClock& clock,
                                     ByteSink onData, IConnectionListener& listener, ConnectionSettings settings)
    : detector_(enumerator),
      port_(port),
      clock_(clock),
      onData_(std::move(onData)),
      listener_(listener),
      settings_(settings) {}

ConnectionManager::~ConnectionManager() { shutdown(); }

ConnectionState ConnectionManager::state() const { return state_; }

void ConnectionManager::start() {
    if (worker_.joinable()) return;
    control_ = std::make_shared<WorkerControl>();
    worker_ = std::jthread([this, control = control_](std::stop_token stop) {
        run(stop, *control);
        std::lock_guard lock(control->mutex);
        control->finished = true;
        control->finishedChanged.notify_all();
    });
}

bool ConnectionManager::shutdown() {
    if (!worker_.joinable()) return true;
    worker_.request_stop();
    bool finished = false;
    {
        std::unique_lock lock(control_->mutex);
        finished = control_->finishedChanged.wait_for(lock, settings_.shutdownTimeout,
                                                      [&] { return control_->finished; });
    }
    if (finished) {
        worker_.join();
        return true;
    }
    control_->abandoned = true;  // checked by the worker after every device call
    worker_.detach();
    state_ = ConnectionState::Searching;
    return false;
}

void ConnectionManager::run(std::stop_token stop, const WorkerControl& control) {
    TimePoint nextAttempt = clock_.now();
    while (!stop.stop_requested() && !control.abandoned) {
        waitUntil(nextAttempt, stop);
        if (stop.stop_requested()) return;
        nextAttempt = clock_.now() + settings_.retryInterval;
        connectAndStream(stop, control);
    }
}

void ConnectionManager::connectAndStream(std::stop_token stop, const WorkerControl& control) {
    const auto detection = detector_.findBoard();
    if (control.abandoned) return;
    if (!detection.ok()) {
        listener_.onConnectError(detection.error());
        return;
    }
    if (!detection.value()) return;  // board not attached yet: keep searching
    const SerialPortInfo board = *detection.value();

    const Status opened = port_.open(board.portName, settings_.baudRate);
    if (control.abandoned) return;
    if (!opened.ok()) {
        listener_.onConnectError(opened.error());
        return;
    }
    state_ = ConnectionState::Connected;
    listener_.onConnected(board);  // before any data: listeners reset per-connection state

    const std::string reason = streamUntilFailureOrStop(stop, control);
    if (control.abandoned) return;
    const Status closed = port_.close();
    if (control.abandoned) return;
    state_ = ConnectionState::Searching;
    listener_.onDisconnected(closed.ok() ? reason : reason + "; " + closed.error());
}

std::string ConnectionManager::streamUntilFailureOrStop(std::stop_token stop, const WorkerControl& control) {
    std::vector<std::uint8_t> buffer(kReadChunkBytes);
    while (!stop.stop_requested()) {
        const ReadResult result = port_.read(buffer, kReadTimeout);
        if (control.abandoned) return {};
        if (result.status == ReadStatus::Data) {
            onData_(std::span<const std::uint8_t>(buffer.data(), result.bytesRead));
        } else if (result.status == ReadStatus::Failed) {
            return result.error;
        }
    }
    return kShutdownReason;
}

void ConnectionManager::waitUntil(TimePoint deadline, std::stop_token stop) const {
    while (clock_.now() < deadline && !stop.stop_requested()) std::this_thread::sleep_for(kIdlePoll);
}

}  // namespace coral