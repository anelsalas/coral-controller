#pragma once
#include "filters/FilterBase.h"

namespace coral {

class FlipFilter final : public FilterBase {
public:
    static constexpr std::string_view kName = "Flip";
    static constexpr std::size_t kDirectionParameter = 0;
    enum DirectionChoice { kHorizontal = 0, kVertical = 1, kBoth = 2 };

    FlipFilter();
    cv::Mat apply(const cv::Mat& input) const override;
};

}  // namespace coral