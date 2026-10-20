#include "status/StatusModel.h"

namespace coral {

void StatusModel::onConnected(const SerialPortInfo& port) {
    std::lock_guard lock(mutex_);
    state_.link = LinkState::Connected;
    state_.portName = port.portName;
    state_.protocol = kDetectingProtocol;
    state_.boardApp = kUnknownBoardApp;  // only the binary protocol announces the app (PROTO-006)
    state_.lastError.clear();
}

void StatusModel::onDisconnected(const std::string& reason) {
    std::lock_guard lock(mutex_);
    state_.link = LinkState::Searching;
    state_.lastError = reason;
}

void StatusModel::onConnectError(const std::string& message) { onError(message); }

void StatusModel::onError(const std::string& message) {
    std::lock_guard lock(mutex_);
    state_.lastError = message;
}

void StatusModel::onFrameDecoded(TimePoint receivedAt) {
    std::lock_guard lock(mutex_);
    ++state_.framesDecoded;
    fps_.recordFrame(receivedAt);
}

void StatusModel::onParseError(ParseErrorKind kind, const std::string& detail) {
    std::lock_guard lock(mutex_);
    if (kind == ParseErrorKind::InvalidBase64) ++state_.invalidBase64Frames;
    if (kind == ParseErrorKind::OverlongLine) ++state_.overlongLines;
    state_.lastError = detail;
}

void StatusModel::onDecodeError(const std::string& message) {
    std::lock_guard lock(mutex_);
    ++state_.jpegDecodeErrors;
    state_.lastError = message;
}

void StatusModel::onLogLine(const std::string& line) {
    std::lock_guard lock(mutex_);
    logLines_.push_back(line);
    if (logLines_.size() > kMaxLogLines) logLines_.pop_front();
}

void StatusModel::onProtocolDetected(const std::string& protocolName) {
    std::lock_guard lock(mutex_);
    state_.protocol = protocolName;
}

void StatusModel::onBoardIdentity(const std::string& appName, const std::string& appVersion) {
    std::lock_guard lock(mutex_);
    state_.boardApp = appName + " " + appVersion;
}

void StatusModel::onIncompatibleProtocol(unsigned boardProtocolVersion) {
    std::lock_guard lock(mutex_);
    ++state_.rejectedPackets;
    state_.boardApp = "incompatible firmware (protocol v" + std::to_string(boardProtocolVersion) + ")";
    state_.lastError = state_.boardApp;
}

void StatusModel::onPacketRejected(const std::string& detail) {
    std::lock_guard lock(mutex_);
    ++state_.rejectedPackets;
    state_.lastError = detail;
}

void StatusModel::onDroppedPackets(std::uint64_t totalDropped) {
    std::lock_guard lock(mutex_);
    state_.droppedPackets = totalDropped;
}

StatusSnapshot StatusModel::snapshot(TimePoint now) {
    std::lock_guard lock(mutex_);
    StatusSnapshot copy = state_;
    copy.framesPerSecond = fps_.framesPerSecond(now);
    copy.recentLogLines.assign(logLines_.begin(), logLines_.end());
    return copy;
}

}  // namespace coral