#pragma once
#include "imaging/JpegDecoder.h"
#include "imaging/LatestFrameBuffer.h"
#include "protocol/TextFrameParser.h"
#include "status/StatusModel.h"

namespace coral {

// Reader-thread side of the viewer: bytes -> text protocol -> JPEG -> latest frame.
class ViewerSession final : public ITextProtocolListener {
public:
    ViewerSession(LatestFrameBuffer& frames, StatusModel& status, const IClock& clock);

    void onBytes(std::span<const std::uint8_t> bytes);

    void onFrame(std::vector<std::uint8_t> jpeg) override;
    void onLogLine(const std::string& line) override;
    void onParseError(ParseErrorKind kind, const std::string& detail) override;

private:
    LatestFrameBuffer& frames_;
    StatusModel& status_;
    const IClock& clock_;
    TextFrameParser parser_;
    JpegDecoder decoder_;
    std::uint64_t nextSequence_ = 1;
};

}  // namespace coral