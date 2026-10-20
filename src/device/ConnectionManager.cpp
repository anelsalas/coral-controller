#include "device/ConnectionManager.h"

namespace coral {

ConnectionManager::ConnectionManager(IPortEnumerator& enumerator, ISerialPort& port, const IClock& clock,
                                     ByteSink onData, IConnectionListener& listener, ConnectionSettings settings)
    : detector_(enumerator),
      port_(port),
      clock_(clock),
      onData_(std::move(onData)),
      listener_(listener),
      settings_(settings) {}

ConnectionManager::~ConnectionManager() { shutdown(); }

void ConnectionManager::tick() {
    if (state_ == ConnectionState::Connected) {
        handleReadFailureIfAny();
    } else {
        tryConnect();
    }
}

ConnectionState ConnectionManager::state() const { return state_; }

void ConnectionManager::shutdown() {
    if (state_ == ConnectionState::Connected) disconnect("application shutdown");
}

void ConnectionManager::tryConnect() {
    const TimePoint now = clock_.now();
    if (now < nextAttempt_) return;
    nextAttempt_ = now + settings_.retryInterval;

    const auto detection = detector_.findBoard();
    if (!detection.ok()) {
        listener_.onConnectError(detection.error());
        return;
    }
    if (!detection.value()) return;  // board not attached yet: keep searching
    const SerialPortInfo board = *detection.value();

    const Status opened = port_.open(board.portName, settings_.baudRate);
    if (!opened.ok()) {
        listener_.onConnectError(opened.error());
        return;
    }
    {
        std::lock_guard lock(failureMutex_);
        pendingReadFailure_.reset();
    }
    reader_ = std::make_unique<SerialReader>(port_, onData_,
                                             [this](const std::string& message) { recordReadFailure(message); });
    reader_->start();
    state_ = ConnectionState::Connected;
    listener_.onConnected(board);
}

void ConnectionManager::recordReadFailure(const std::string& message) {
    std::lock_guard lock(failureMutex_);
    pendingReadFailure_ = message;
}

void ConnectionManager::handleReadFailureIfAny() {
    std::optional<std::string> failure;
    {
        std::lock_guard lock(failureMutex_);
        failure.swap(pendingReadFailure_);
    }
    if (failure) disconnect(*failure);
}

void ConnectionManager::disconnect(const std::string& reason) {
    if (reader_) reader_->stop();
    reader_.reset();
    const Status closed = port_.close();
    state_ = ConnectionState::Searching;
    nextAttempt_ = clock_.now() + settings_.retryInterval;
    listener_.onDisconnected(closed.ok() ? reason : reason + "; " + closed.error());
}

}  // namespace coral