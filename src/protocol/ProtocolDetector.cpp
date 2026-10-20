#include "protocol/ProtocolDetector.h"

#include "protocol/CoralProtocol.h"

#include <algorithm>

namespace coral {
namespace {
constexpr std::size_t kNotFound = static_cast<std::size_t>(-1);

std::size_t find(const std::vector<std::uint8_t>& haystack, const std::uint8_t* needle, std::size_t length) {
    const auto position = std::search(haystack.begin(), haystack.end(), needle, needle + length);
    return position == haystack.end() ? kNotFound : static_cast<std::size_t>(position - haystack.begin());
}
}  // namespace

StreamProtocol ProtocolDetector::feed(std::span<const std::uint8_t> bytes) {
    if (protocol_ != StreamProtocol::Unknown) return protocol_;
    probe_.insert(probe_.end(), bytes.begin(), bytes.end());
    const std::size_t magicAt = find(probe_, protocol::kMagic, protocol::kMagicBytes);
    const std::size_t textAt =
        find(probe_, reinterpret_cast<const std::uint8_t*>(kTextMarker.data()), kTextMarker.size());
    if (magicAt != kNotFound && (textAt == kNotFound || magicAt < textAt)) {
        decide(StreamProtocol::Binary, magicAt);
    } else if (textAt != kNotFound) {
        decide(StreamProtocol::Text, 0);  // keep earlier lines: they are board log output
    } else if (probe_.size() > kMaxProbeBytes) {
        decide(StreamProtocol::Text, 0);  // plain console output: show it as log
    }
    return protocol_;
}

void ProtocolDetector::decide(StreamProtocol protocol, std::size_t replayFrom) {
    protocol_ = protocol;
    probe_.erase(probe_.begin(), probe_.begin() + static_cast<std::ptrdiff_t>(replayFrom));
}

std::vector<std::uint8_t> ProtocolDetector::takeBufferedBytes() {
    std::vector<std::uint8_t> bytes;
    bytes.swap(probe_);
    return bytes;
}

StreamProtocol ProtocolDetector::protocol() const { return protocol_; }

void ProtocolDetector::reset() {
    protocol_ = StreamProtocol::Unknown;
    probe_.clear();
}

}  // namespace coral