#include "filters/BuiltinFilters.h"

#include "filters/BlurFilter.h"
#include "filters/ColorMapFilter.h"
#include "filters/EdgeFilter.h"
#include "filters/FlipFilter.h"
#include "filters/RotateFilter.h"
#include "filters/ThresholdFilter.h"

namespace coral {

// To add a filter: write one FilterBase subclass and add one line here. Order = default pipeline order.
void registerBuiltinFilters(FilterRegistry& registry) {
    registry.add<RotateFilter>();
    registry.add<FlipFilter>();
    registry.add<BlurFilter>();
    registry.add<EdgeFilter>();
    registry.add<ThresholdFilter>();
    registry.add<ColorMapFilter>();
}

}  // namespace coral