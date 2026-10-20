#pragma once
#include "filters/FilterBase.h"

namespace coral {

class RotateFilter final : public FilterBase {
public:
    static constexpr std::string_view kName = "Rotate";
    static constexpr std::size_t kAngleParameter = 0;
    enum AngleChoice { kClockwise90 = 0, kHalfTurn = 1, kCounterClockwise90 = 2 };

    RotateFilter();
    cv::Mat apply(const cv::Mat& input) const override;
};

}  // namespace coral