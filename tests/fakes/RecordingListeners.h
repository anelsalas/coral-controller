#pragma once
#include "device/ConnectionManager.h"
#include "protocol/TextFrameParser.h"

#include <mutex>
#include <string>
#include <vector>

namespace coral::testing {

class RecordingTextListener final : public ITextProtocolListener {
public:
    void onFrame(std::vector<std::uint8_t> jpeg) override { frames.push_back(std::move(jpeg)); }
    void onLogLine(const std::string& line) override { logLines.push_back(line); }
    void onParseError(ParseErrorKind kind, const std::string&) override { errors.push_back(kind); }

    std::vector<std::vector<std::uint8_t>> frames;
    std::vector<std::string> logLines;
    std::vector<ParseErrorKind> errors;
};

class RecordingConnectionListener final : public IConnectionListener {
public:
    void onConnected(const SerialPortInfo& port) override {
        std::lock_guard lock(mutex_);
        connectedPorts_.push_back(port.portName);
    }
    void onDisconnected(const std::string& reason) override {
        std::lock_guard lock(mutex_);
        disconnectReasons_.push_back(reason);
    }
    void onConnectError(const std::string& message) override {
        std::lock_guard lock(mutex_);
        connectErrors_.push_back(message);
    }

    std::size_t connectedCount() const {
        std::lock_guard lock(mutex_);
        return connectedPorts_.size();
    }
    std::vector<std::string> disconnectReasons() const {
        std::lock_guard lock(mutex_);
        return disconnectReasons_;
    }

private:
    mutable std::mutex mutex_;
    std::vector<std::string> connectedPorts_;
    std::vector<std::string> disconnectReasons_;
    std::vector<std::string> connectErrors_;
};

}  // namespace coral::testing