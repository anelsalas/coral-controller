#include "viewer/ViewerSession.h"

namespace coral {

ViewerSession::ViewerSession(LatestFrameBuffer& frames, StatusModel& status, const IClock& clock)
    : frames_(frames), status_(status), clock_(clock), parser_(*this) {}

void ViewerSession::onBytes(std::span<const std::uint8_t> bytes) { parser_.feed(bytes); }

void ViewerSession::onFrame(std::vector<std::uint8_t> jpeg) {
    auto decoded = decoder_.decode(jpeg);
    if (!decoded.ok()) {
        status_.onDecodeError(decoded.error());
        return;
    }
    const TimePoint now = clock_.now();
    status_.onFrameDecoded(now);
    frames_.publish(Frame{std::move(decoded.value()), now, nextSequence_++});
}

void ViewerSession::onLogLine(const std::string& line) { status_.onLogLine(line); }

void ViewerSession::onParseError(ParseErrorKind kind, const std::string& detail) {
    status_.onParseError(kind, detail);
}

}  // namespace coral