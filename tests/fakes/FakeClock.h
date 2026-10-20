#pragma once
#include "common/Clock.h"

namespace coral::testing {

class FakeClock final : public IClock {
public:
    TimePoint now() const override { return now_; }
    void advance(std::chrono::milliseconds step) { now_ += step; }

private:
    TimePoint now_{std::chrono::hours(1)};
};

}  // namespace coral::testing