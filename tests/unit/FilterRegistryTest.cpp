// VIEW-009: a new filter = one class + one registration line.
#include "filters/FilterRegistry.h"
#include "fakes/TestFilters.h"
#include "ui/FilterPanelModel.h"

#include <gtest/gtest.h>

#include <algorithm>

using namespace coral;

TEST(FilterRegistry, ExposesNewlyRegisteredFilter) {
    FilterRegistry registry;

    registry.add<coral::testing::DoubleFilter>();  // the one registration line

    const auto names = registry.names();
    EXPECT_NE(std::find(names.begin(), names.end(), coral::testing::DoubleFilter::kName), names.end());

    FilterPipeline pipeline;
    for (auto& filter : registry.createAll()) pipeline.append(std::move(filter), false);
    FilterPanelModel model(pipeline);
    ASSERT_EQ(model.filterCount(), 1u);
    EXPECT_EQ(model.filterName(0), coral::testing::DoubleFilter::kName);
}

TEST(FilterRegistry, RejectsDuplicateNames) {
    FilterRegistry registry;
    registry.add<coral::testing::DoubleFilter>();
    EXPECT_THROW(registry.add<coral::testing::DoubleFilter>(), std::logic_error);
}