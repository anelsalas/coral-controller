// VIEW-008: parameter values are clamped to declared ranges.
#include "filters/FilterParameter.h"

#include <gtest/gtest.h>

using namespace coral;

TEST(FilterParameters, ClampToDeclaredRange) {
    auto level = FilterParameter::integer("Level", 128, 0, 255);
    level.setValue(300);
    EXPECT_EQ(level.asInt(), 255);
    level.setValue(-5);
    EXPECT_EQ(level.asInt(), 0);

    auto sigma = FilterParameter::real("Sigma", 1.0, 0.0, 10.0);
    sigma.setValue(42.0);
    EXPECT_DOUBLE_EQ(sigma.asReal(), 10.0);

    auto mode = FilterParameter::choice("Mode", {"A", "B", "C"}, 0);
    mode.setValue(7);
    EXPECT_EQ(mode.asChoiceIndex(), 2);
}

TEST(FilterParameters, IntegerRoundsToNearest) {
    auto size = FilterParameter::integer("Size", 1, 0, 10);
    size.setValue(3.6);
    EXPECT_EQ(size.asInt(), 4);
}

TEST(FilterParameters, BooleanIsZeroOrOne) {
    auto invert = FilterParameter::boolean("Invert", false);
    invert.setValue(0.3);
    EXPECT_TRUE(invert.asBool());
}