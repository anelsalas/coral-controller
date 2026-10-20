#pragma once
#include "filters/FilterPipeline.h"
#include "filters/FilterRegistry.h"
#include "filters/RotateFilter.h"

namespace coral {

// camera_streaming_serial delivers frames rotated by 90 degrees (VIEW-010).
// Direction confirmed on hardware: see REQUIREMENTS.md OQ-4.
inline constexpr int kUprightRotationChoice = RotateFilter::kClockwise90;

// All registered filters in registration order; only the upright rotation is enabled.
FilterPipeline makeDefaultPipeline(const FilterRegistry& registry);

}  // namespace coral