#pragma once
#include "common/Clock.h"

#include <deque>

namespace coral {

// Frame rate over a sliding time window (VIEW-013).
class FpsCounter {
public:
    static constexpr std::chrono::milliseconds kDefaultWindow{2000};

    explicit FpsCounter(std::chrono::milliseconds window = kDefaultWindow);
    void recordFrame(TimePoint receivedAt);
    double framesPerSecond(TimePoint now);

private:
    void dropFramesOlderThan(TimePoint oldestAllowed);

    std::chrono::milliseconds window_;
    std::deque<TimePoint> frameTimes_;
};

}  // namespace coral