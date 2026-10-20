// VIEW-006: ordered, toggleable filter pipeline with explicit failure reporting.
#include "filters/FilterPipeline.h"
#include "fakes/TestFilters.h"

#include <gtest/gtest.h>

using namespace coral;
using namespace coral::testing;

namespace {
cv::Mat singlePixel(double value) { return cv::Mat(1, 1, CV_64FC1, cv::Scalar(value)); }
double pixelOf(const cv::Mat& image) { return image.at<double>(0, 0); }
}  // namespace

TEST(FilterPipeline, AppliesEnabledFiltersInOrder) {
    FilterPipeline pipeline;
    pipeline.append(std::make_unique<AddConstantFilter>(10), true);
    pipeline.append(std::make_unique<DoubleFilter>(), true);

    const auto output = pipeline.apply(singlePixel(1));

    EXPECT_FALSE(output.error.has_value());
    EXPECT_DOUBLE_EQ(pixelOf(output.image), (1 + 10) * 2);
}

TEST(FilterPipeline, SkipsDisabledFilters) {
    FilterPipeline pipeline;
    pipeline.append(std::make_unique<AddConstantFilter>(10), false);
    pipeline.append(std::make_unique<DoubleFilter>(), true);

    const auto output = pipeline.apply(singlePixel(1));

    EXPECT_DOUBLE_EQ(pixelOf(output.image), 2);
}

TEST(FilterPipeline, ReportsFailingFilterByName) {
    FilterPipeline pipeline;
    pipeline.append(std::make_unique<DoubleFilter>(), true);
    pipeline.append(std::make_unique<FailingFilter>(), true);

    const auto output = pipeline.apply(singlePixel(3));

    ASSERT_TRUE(output.error.has_value());
    EXPECT_EQ(output.error->filterName, FailingFilter::kName);
    EXPECT_NE(output.error->message.find("deliberate failure"), std::string::npos);
    EXPECT_DOUBLE_EQ(pixelOf(output.image), 3) << "the unfiltered frame is shown on failure";
}

TEST(FilterPipeline, MovesStageToNewPosition) {
    FilterPipeline pipeline;
    pipeline.append(std::make_unique<AddConstantFilter>(10), true);
    pipeline.append(std::make_unique<DoubleFilter>(), true);

    pipeline.move(1, 0);

    EXPECT_EQ(pipeline.filterAt(0).name(), DoubleFilter::kName);
    EXPECT_DOUBLE_EQ(pixelOf(pipeline.apply(singlePixel(1)).image), 1 * 2 + 10);
}