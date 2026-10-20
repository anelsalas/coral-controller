// VIEW-013: received fps over a sliding window.
#include "status/FpsCounter.h"

#include <gtest/gtest.h>

using namespace coral;
using namespace std::chrono_literals;

TEST(FpsCounter, ComputesRateOverWindow) {
    FpsCounter counter(2s);
    const TimePoint start{};
    for (int i = 0; i < 10; ++i) counter.recordFrame(start + i * 100ms);

    EXPECT_NEAR(counter.framesPerSecond(start + 900ms), 10.0, 1e-9);
    EXPECT_DOUBLE_EQ(counter.framesPerSecond(start + 10s), 0.0) << "old frames leave the window";
}

TEST(FpsCounter, ReportsZeroWithFewerThanTwoFrames) {
    FpsCounter counter(2s);
    counter.recordFrame(TimePoint{});
    EXPECT_DOUBLE_EQ(counter.framesPerSecond(TimePoint{}), 0.0);
}