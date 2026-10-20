#pragma once
#include "imaging/Frame.h"

#include <mutex>
#include <optional>

namespace coral {

// Single-slot hand-over between the reader thread and the UI thread. A newer frame replaces
// an unconsumed one, so display latency never grows (VIEW-005, PERF-003).
class LatestFrameBuffer {
public:
    void publish(Frame frame);
    std::optional<Frame> takeLatest();
    std::uint64_t droppedCount() const;

private:
    mutable std::mutex mutex_;
    std::optional<Frame> latest_;
    std::uint64_t dropped_ = 0;
};

}  // namespace coral