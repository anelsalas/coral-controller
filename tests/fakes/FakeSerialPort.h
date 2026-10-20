#pragma once
#include "device/ISerialPort.h"

#include <algorithm>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

namespace coral::testing {

// Scripted serial port: each queued step is returned by one read() call.
// When the script is empty, read() behaves like a quiet line (short wait, Timeout).
class FakeSerialPort final : public ISerialPort {
public:
    void queueData(const std::string& text) {
        std::lock_guard lock(mutex_);
        steps_.push_back(Step{ReadStatus::Data, text, {}});
    }

    void queueFailure(const std::string& message) {
        std::lock_guard lock(mutex_);
        steps_.push_back(Step{ReadStatus::Failed, {}, message});
    }

    void setReadDelay(std::chrono::milliseconds delay) { readDelay_ = delay; }

    Status open(const std::string& portName, unsigned baudRate) override {
        std::lock_guard lock(mutex_);
        openedPortName_ = portName;
        openedBaudRate_ = baudRate;
        isOpen_ = true;
        ++openCount_;
        return okStatus();
    }

    Status close() override {
        std::lock_guard lock(mutex_);
        isOpen_ = false;
        return okStatus();
    }

    bool isOpen() const override {
        std::lock_guard lock(mutex_);
        return isOpen_;
    }

    ReadResult read(std::span<std::uint8_t> buffer, std::chrono::milliseconds timeout) override {
        std::this_thread::sleep_for(readDelay_);
        std::unique_lock lock(mutex_);
        if (steps_.empty()) {
            lock.unlock();
            std::this_thread::sleep_for(std::min(timeout, kQuietLineWait));
            return ReadResult{ReadStatus::Timeout, 0, {}};
        }
        Step step = steps_.front();
        steps_.pop_front();
        if (step.status != ReadStatus::Data) {
            return ReadResult{step.status, 0, step.error};
        }
        const std::size_t count = std::min(buffer.size(), step.data.size());
        std::copy_n(step.data.begin(), count, buffer.begin());
        return ReadResult{ReadStatus::Data, count, {}};
    }

    int openCount() const {
        std::lock_guard lock(mutex_);
        return openCount_;
    }
    std::string openedPortName() const {
        std::lock_guard lock(mutex_);
        return openedPortName_;
    }
    unsigned openedBaudRate() const {
        std::lock_guard lock(mutex_);
        return openedBaudRate_;
    }

private:
    static constexpr std::chrono::milliseconds kQuietLineWait{2};

    struct Step {
        ReadStatus status;
        std::string data;
        std::string error;
    };

    mutable std::mutex mutex_;
    std::deque<Step> steps_;
    std::chrono::milliseconds readDelay_{0};
    bool isOpen_ = false;
    int openCount_ = 0;
    std::string openedPortName_;
    unsigned openedBaudRate_ = 0;
};

}  // namespace coral::testing