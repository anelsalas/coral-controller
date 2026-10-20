#pragma once
#include "filters/FilterParameter.h"

#include <opencv2/core.hpp>

#include <string_view>
#include <vector>

namespace coral {

// One image operation. apply() may throw (e.g. cv::Exception); FilterPipeline reports it by name.
class IFilter {
public:
    virtual ~IFilter() = default;
    virtual std::string_view name() const = 0;
    virtual std::vector<FilterParameter>& parameters() = 0;
    virtual const std::vector<FilterParameter>& parameters() const = 0;
    virtual cv::Mat apply(const cv::Mat& input) const = 0;
};

}  // namespace coral