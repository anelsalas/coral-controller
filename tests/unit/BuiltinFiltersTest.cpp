// VIEW-007: built-in filters.
#include "filters/BlurFilter.h"
#include "filters/ColorMapFilter.h"
#include "filters/EdgeFilter.h"
#include "filters/FlipFilter.h"
#include "filters/RotateFilter.h"
#include "filters/ThresholdFilter.h"

#include <gtest/gtest.h>

using namespace coral;

namespace {
// 2 rows x 3 columns gray image with distinct values:
//   1 2 3
//   4 5 6
cv::Mat numberedImage() { return (cv::Mat_<std::uint8_t>(2, 3) << 1, 2, 3, 4, 5, 6); }
}  // namespace

TEST(RotateFilter, Rotates90Clockwise) {
    RotateFilter filter;
    filter.parameters()[RotateFilter::kAngleParameter].setValue(RotateFilter::kClockwise90);

    const cv::Mat rotated = filter.apply(numberedImage());

    // Clockwise: first row becomes the left column read bottom-up.
    const cv::Mat expected = (cv::Mat_<std::uint8_t>(3, 2) << 4, 1, 5, 2, 6, 3);
    ASSERT_EQ(rotated.size(), expected.size());
    EXPECT_EQ(cv::countNonZero(rotated != expected), 0);
}

TEST(FlipFilter, FlipsHorizontally) {
    FlipFilter filter;
    filter.parameters()[FlipFilter::kDirectionParameter].setValue(FlipFilter::kHorizontal);

    const cv::Mat flipped = filter.apply(numberedImage());

    const cv::Mat expected = (cv::Mat_<std::uint8_t>(2, 3) << 3, 2, 1, 6, 5, 4);
    EXPECT_EQ(cv::countNonZero(flipped != expected), 0);
}

TEST(BlurFilter, SmoothsImpulse) {
    cv::Mat impulse = cv::Mat::zeros(9, 9, CV_8UC1);
    impulse.at<std::uint8_t>(4, 4) = 255;
    BlurFilter filter;

    const cv::Mat blurred = filter.apply(impulse);

    EXPECT_LT(blurred.at<std::uint8_t>(4, 4), 255);
    EXPECT_GT(blurred.at<std::uint8_t>(4, 5), 0);
}

TEST(BlurFilter, ForcesOddKernelSize) {
    BlurFilter filter;
    filter.parameters()[BlurFilter::kKernelSizeParameter].setValue(4);
    EXPECT_NO_THROW(filter.apply(cv::Mat::zeros(9, 9, CV_8UC1)));
}

TEST(EdgeFilter, DetectsStepEdge) {
    cv::Mat step = cv::Mat::zeros(20, 20, CV_8UC3);
    step(cv::Rect(10, 0, 10, 20)).setTo(cv::Scalar::all(255));
    EdgeFilter filter;

    const cv::Mat edges = filter.apply(step);

    EXPECT_EQ(edges.channels(), 1);
    EXPECT_GT(cv::countNonZero(edges.col(9) | edges.col(10)), 0);
    EXPECT_EQ(cv::countNonZero(edges.col(2)), 0);
}

TEST(ThresholdFilter, BinarizesAtLevel) {
    const cv::Mat ramp = (cv::Mat_<std::uint8_t>(1, 4) << 10, 100, 150, 250);
    ThresholdFilter filter;
    filter.parameters()[ThresholdFilter::kLevelParameter].setValue(120);

    const cv::Mat binary = filter.apply(ramp);

    const cv::Mat expected = (cv::Mat_<std::uint8_t>(1, 4) << 0, 0, 255, 255);
    EXPECT_EQ(cv::countNonZero(binary != expected), 0);
}

TEST(ColorMapFilter, ProducesThreeChannels) {
    ColorMapFilter filter;
    const cv::Mat colored = filter.apply(numberedImage());
    EXPECT_EQ(colored.channels(), 3);
    EXPECT_EQ(colored.size(), numberedImage().size());
}