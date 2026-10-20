#pragma once
#include "common/Clock.h"

#include <atomic>

namespace coral::testing {

// Thread-safe: the connection worker reads it while the test advances it.
class FakeClock final : public IClock {
public:
    TimePoint now() const override { return TimePoint(TimePoint::duration(now_.load())); }
    void advance(std::chrono::milliseconds step) {
        now_ += std::chrono::duration_cast<TimePoint::duration>(step).count();
    }

private:
    std::atomic<TimePoint::rep> now_{std::chrono::duration_cast<TimePoint::duration>(std::chrono::hours(1)).count()};
};

}  // namespace coral::testing