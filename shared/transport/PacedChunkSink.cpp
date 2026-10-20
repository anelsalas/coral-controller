#include "transport/PacedChunkSink.h"

namespace coral {
namespace transport {

PacedChunkSink::PacedChunkSink(IChunkPort& port, PacingConfig config) : port_(port), config_(config) {}

bool PacedChunkSink::write(const std::uint8_t* data, std::size_t length) {
    while (length > 0) {
        const std::size_t chunk = length < config_.maxChunkBytes ? length : config_.maxChunkBytes;
        sendPaced(data, chunk);
        data += chunk;
        length -= chunk;
    }
    return true;
}

void PacedChunkSink::sendPaced(const std::uint8_t* data, std::size_t length) {
    if (hasSent_) {
        const std::uint64_t earliest = lastSendMicros_ + config_.minGapMicros;
        if (port_.nowMicros() < earliest) port_.waitUntilMicros(earliest);
    }
    lastSendMicros_ = port_.nowMicros();
    hasSent_ = true;
    port_.send(data, length);
}

}  // namespace transport
}  // namespace coral