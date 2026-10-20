#include "status/FpsCounter.h"

namespace coral {

FpsCounter::FpsCounter(std::chrono::milliseconds window) : window_(window) {}

void FpsCounter::recordFrame(TimePoint receivedAt) { frameTimes_.push_back(receivedAt); }

double FpsCounter::framesPerSecond(TimePoint now) {
    dropFramesOlderThan(now - window_);
    if (frameTimes_.size() < 2) return 0.0;
    const std::chrono::duration<double> span = frameTimes_.back() - frameTimes_.front();
    if (span.count() <= 0.0) return 0.0;
    return static_cast<double>(frameTimes_.size() - 1) / span.count();
}

void FpsCounter::dropFramesOlderThan(TimePoint oldestAllowed) {
    while (!frameTimes_.empty() && frameTimes_.front() < oldestAllowed) frameTimes_.pop_front();
}

}  // namespace coral