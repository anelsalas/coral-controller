#pragma once
#include "filters/FilterBase.h"

namespace coral {

class ThresholdFilter final : public FilterBase {
public:
    static constexpr std::string_view kName = "Threshold";
    static constexpr std::size_t kLevelParameter = 0;
    static constexpr std::size_t kInvertParameter = 1;

    ThresholdFilter();
    cv::Mat apply(const cv::Mat& input) const override;
};

}  // namespace coral