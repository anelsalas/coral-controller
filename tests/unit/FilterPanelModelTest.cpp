// VIEW-008: UI model edits filter parameters; effect is visible on the next frame.
#include "filters/ThresholdFilter.h"
#include "ui/FilterPanelModel.h"

#include <gtest/gtest.h>

using namespace coral;

TEST(FilterPanelModel, UpdatesFilterParameter) {
    FilterPipeline pipeline;
    pipeline.append(std::make_unique<ThresholdFilter>(), true);
    FilterPanelModel model(pipeline);
    const cv::Mat gray(1, 1, CV_8UC1, cv::Scalar(100));
    ASSERT_EQ(pipeline.apply(gray).image.at<std::uint8_t>(0, 0), 0);

    model.setParameter(0, ThresholdFilter::kLevelParameter, 50);

    EXPECT_EQ(model.parameters(0)[ThresholdFilter::kLevelParameter].asInt(), 50);
    EXPECT_EQ(pipeline.apply(gray).image.at<std::uint8_t>(0, 0), 255);
}

TEST(FilterPanelModel, TogglesAndReordersFilters) {
    FilterPipeline pipeline;
    pipeline.append(std::make_unique<ThresholdFilter>(), true);
    pipeline.append(std::make_unique<ThresholdFilter>(), true);
    FilterPanelModel model(pipeline);

    model.setEnabled(1, false);
    model.moveUp(1);

    EXPECT_FALSE(model.isEnabled(0));
    EXPECT_TRUE(model.isEnabled(1));
}