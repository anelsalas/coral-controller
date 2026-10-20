#include "SequenceTracker.h"

namespace coral {
namespace protocol {

void SequenceTracker::observe(std::uint32_t sequence) {
    if (hasPrevious_) {
        const std::uint32_t gap = sequence - expected_;  // modulo 2^32: wrap-around is a gap of 0
        if (gap <= kMaxForwardGap) dropped_ += gap;
    }
    hasPrevious_ = true;
    expected_ = sequence + 1;
}

void SequenceTracker::restart() { hasPrevious_ = false; }

std::uint64_t SequenceTracker::droppedPackets() const { return dropped_; }

}  // namespace protocol
}  // namespace coral