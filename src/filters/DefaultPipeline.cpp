#include "filters/DefaultPipeline.h"

namespace coral {

FilterPipeline makeDefaultPipeline(const FilterRegistry& registry) {
    FilterPipeline pipeline;
    for (auto& filter : registry.createAll()) {
        const bool isRotation = filter->name() == RotateFilter::kName;
        if (isRotation) filter->parameters()[RotateFilter::kAngleParameter].setValue(kUprightRotationChoice);
        pipeline.append(std::move(filter), isRotation);
    }
    return pipeline;
}

}  // namespace coral