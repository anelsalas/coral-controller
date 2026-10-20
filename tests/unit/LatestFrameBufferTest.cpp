// VIEW-005, PERF-003: only the newest frame is displayed.
#include "imaging/LatestFrameBuffer.h"

#include <gtest/gtest.h>

using namespace coral;

namespace {
Frame frameWithSequence(std::uint64_t sequence) {
    return Frame{cv::Mat(2, 2, CV_8UC3, cv::Scalar::all(0)), TimePoint{}, sequence};
}
}  // namespace

TEST(LatestFrameBuffer, KeepsOnlyNewest) {
    LatestFrameBuffer buffer;

    buffer.publish(frameWithSequence(1));
    buffer.publish(frameWithSequence(2));
    buffer.publish(frameWithSequence(3));
    const auto latest = buffer.takeLatest();

    ASSERT_TRUE(latest.has_value());
    EXPECT_EQ(latest->sequence, 3u);
    EXPECT_EQ(buffer.droppedCount(), 2u);
    EXPECT_FALSE(buffer.takeLatest().has_value()) << "a frame is handed out only once";
}