#pragma once
#include "device/ConnectionManager.h"
#include "protocol/TextFrameParser.h"
#include "status/FpsCounter.h"

#include <deque>
#include <mutex>
#include <string>
#include <vector>

namespace coral {

enum class LinkState { Searching, Connected };

struct StatusSnapshot {
    LinkState link = LinkState::Searching;
    std::string portName;
    std::string protocol;
    std::string boardApp;
    double framesPerSecond = 0.0;
    std::uint64_t framesDecoded = 0;
    std::uint64_t invalidBase64Frames = 0;
    std::uint64_t overlongLines = 0;
    std::uint64_t jpegDecodeErrors = 0;
    std::string lastError;
    std::vector<std::string> recentLogLines;
};

// Thread-safe aggregation of everything the status panel shows (VIEW-013).
// Written from the reader thread and the UI thread, read by the UI via snapshot().
class StatusModel final : public IConnectionListener {
public:
    static constexpr std::size_t kMaxLogLines = 200;
    static constexpr const char* kTextProtocolName = "text (FRAME:base64)";
    static constexpr const char* kUnknownBoardApp = "unknown";

    void onConnected(const SerialPortInfo& port) override;
    void onDisconnected(const std::string& reason) override;
    void onConnectError(const std::string& message) override;

    void onFrameDecoded(TimePoint receivedAt);
    void onParseError(ParseErrorKind kind, const std::string& detail);
    void onDecodeError(const std::string& message);
    void onLogLine(const std::string& line);
    void onError(const std::string& message);

    StatusSnapshot snapshot(TimePoint now);

private:
    std::mutex mutex_;
    StatusSnapshot state_;
    FpsCounter fps_;
    std::deque<std::string> logLines_;
};

}  // namespace coral