#include "protocol/TextFrameParser.h"

#include "protocol/Base64.h"

namespace coral {
namespace {
constexpr char kLineFeed = '\n';
constexpr char kCarriageReturn = '\r';
}  // namespace

TextFrameParser::TextFrameParser(ITextProtocolListener& listener, std::size_t maxLineLength)
    : listener_(listener), maxLineLength_(maxLineLength) {}

const ParseCounters& TextFrameParser::counters() const { return counters_; }

void TextFrameParser::reset() {
    line_.clear();
    discardingUntilNewline_ = false;
}

void TextFrameParser::feed(std::span<const std::uint8_t> bytes) {
    for (const std::uint8_t value : bytes) acceptCharacter(static_cast<char>(value));
}

void TextFrameParser::acceptCharacter(char character) {
    if (character == kLineFeed) {
        if (discardingUntilNewline_) {
            discardingUntilNewline_ = false;
        } else {
            completeLine();
        }
        line_.clear();
        return;
    }
    if (discardingUntilNewline_) return;
    line_.push_back(character);
    if (line_.size() > maxLineLength_) rejectOverlongLine();
}

void TextFrameParser::rejectOverlongLine() {
    ++counters_.overlongLines;
    listener_.onParseError(ParseErrorKind::OverlongLine,
                           "line longer than " + std::to_string(maxLineLength_) + " bytes discarded");
    line_.clear();
    discardingUntilNewline_ = true;
}

void TextFrameParser::completeLine() {
    std::string_view line = line_;
    if (!line.empty() && line.back() == kCarriageReturn) line.remove_suffix(1);
    if (line.empty()) return;
    if (line.starts_with(kFramePrefix)) {
        handleFrameLine(line.substr(kFramePrefix.size()));
        return;
    }
    ++counters_.logLines;
    listener_.onLogLine(std::string(line));
}

void TextFrameParser::handleFrameLine(std::string_view base64Payload) {
    auto jpeg = decodeBase64(base64Payload);
    if (!jpeg || jpeg->empty()) {
        ++counters_.invalidBase64;
        listener_.onParseError(ParseErrorKind::InvalidBase64,
                               "invalid base64 payload (" + std::to_string(base64Payload.size()) + " chars)");
        return;
    }
    ++counters_.frames;
    listener_.onFrame(std::move(*jpeg));
}

}  // namespace coral