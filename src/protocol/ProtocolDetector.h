#pragma once
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace coral {

enum class StreamProtocol { Unknown, Text, Binary };

// Decides, per connection, whether the board speaks the text protocol ("FRAME:" lines) or the
// binary protocol (packets starting with "CRLP"): whichever marker appears first (PROTO-009).
// Bytes seen while deciding are kept and handed to the chosen decoder.
class ProtocolDetector {
public:
    static constexpr std::size_t kMaxProbeBytes = 64 * 1024;
    static constexpr std::string_view kTextMarker = "FRAME:";

    StreamProtocol feed(std::span<const std::uint8_t> bytes);
    std::vector<std::uint8_t> takeBufferedBytes();
    StreamProtocol protocol() const;
    void reset();

private:
    void decide(StreamProtocol protocol, std::size_t replayFrom);

    StreamProtocol protocol_ = StreamProtocol::Unknown;
    std::vector<std::uint8_t> probe_;
};

}  // namespace coral