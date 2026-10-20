// VIEW-010: rotation correction is preconfigured; enabled state follows the hardware finding (OQ-4).
#include "filters/BuiltinFilters.h"
#include "filters/DefaultPipeline.h"
#include "filters/RotateFilter.h"

#include <gtest/gtest.h>

using namespace coral;

TEST(DefaultPipeline, IncludesRotationCorrection) {
    FilterRegistry registry;
    registerBuiltinFilters(registry);

    FilterPipeline pipeline = makeDefaultPipeline(registry);

    ASSERT_GE(pipeline.size(), 6u);
    EXPECT_EQ(pipeline.filterAt(0).name(), RotateFilter::kName);
    EXPECT_EQ(pipeline.isEnabled(0), kRotationEnabledByDefault);
    EXPECT_EQ(pipeline.filterAt(0).parameters()[RotateFilter::kAngleParameter].asChoiceIndex(),
              kUprightRotationChoice);
    for (std::size_t i = 1; i < pipeline.size(); ++i) {
        EXPECT_FALSE(pipeline.isEnabled(i)) << pipeline.filterAt(i).name();
    }
}