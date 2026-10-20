#pragma once
#include "filters/FilterPipeline.h"
#include "filters/FilterRegistry.h"
#include "filters/RotateFilter.h"

namespace coral {

// VIEW-010 / OQ-4: hardware check on 2026-10-19 showed camera_streaming_serial frames already arrive
// upright, so the rotate stage is preconfigured (90 clockwise) but disabled by default.
inline constexpr int kUprightRotationChoice = RotateFilter::kClockwise90;
inline constexpr bool kRotationEnabledByDefault = false;

// All registered filters in registration order, all disabled except as configured above.
FilterPipeline makeDefaultPipeline(const FilterRegistry& registry);

}  // namespace coral