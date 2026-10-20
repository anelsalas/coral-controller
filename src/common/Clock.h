#pragma once
#include <chrono>

namespace coral {

using SteadyTime = std::chrono::steady_clock;
using TimePoint = SteadyTime::time_point;

// Injected time source so time-dependent logic (retry intervals, fps) is testable.
class IClock {
public:
    virtual ~IClock() = default;
    virtual TimePoint now() const = 0;
};

class SteadyClock final : public IClock {
public:
    TimePoint now() const override { return SteadyTime::now(); }
};

}  // namespace coral