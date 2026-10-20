#pragma once
#include "filters/FilterBase.h"

namespace coral {

// Canny edge detector; output is a single-channel edge map.
class EdgeFilter final : public FilterBase {
public:
    static constexpr std::string_view kName = "Edges (Canny)";
    static constexpr std::size_t kLowThresholdParameter = 0;
    static constexpr std::size_t kHighThresholdParameter = 1;

    EdgeFilter();
    cv::Mat apply(const cv::Mat& input) const override;
};

}  // namespace coral