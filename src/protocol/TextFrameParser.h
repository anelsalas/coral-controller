#pragma once
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace coral {

enum class ParseErrorKind { InvalidBase64, OverlongLine };

struct ParseCounters {
    std::uint64_t frames = 0;
    std::uint64_t logLines = 0;
    std::uint64_t invalidBase64 = 0;
    std::uint64_t overlongLines = 0;
};

class ITextProtocolListener {
public:
    virtual ~ITextProtocolListener() = default;
    virtual void onFrame(std::vector<std::uint8_t> jpeg) = 0;
    virtual void onLogLine(const std::string& line) = 0;
    virtual void onParseError(ParseErrorKind kind, const std::string& detail) = 0;
};

// Incremental parser for the current text protocol: "FRAME:<base64 JPEG>\r\n" (VIEW-003/004).
// Any other line is board log output. Bytes may arrive in arbitrary chunks.
class TextFrameParser {
public:
    static constexpr std::string_view kFramePrefix = "FRAME:";
    static constexpr std::size_t kDefaultMaxLineLength = 1024 * 1024;

    explicit TextFrameParser(ITextProtocolListener& listener, std::size_t maxLineLength = kDefaultMaxLineLength);

    void feed(std::span<const std::uint8_t> bytes);
    void reset();  // drop any partial line (new connection)
    const ParseCounters& counters() const;

private:
    void acceptCharacter(char character);
    void completeLine();
    void handleFrameLine(std::string_view base64Payload);
    void rejectOverlongLine();

    ITextProtocolListener& listener_;
    std::size_t maxLineLength_;
    std::string line_;
    bool discardingUntilNewline_ = false;
    ParseCounters counters_;
};

}  // namespace coral