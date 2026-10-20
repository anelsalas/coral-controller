#pragma once
#include "filters/FilterBase.h"

namespace coral {

// False-color rendering of image intensity.
class ColorMapFilter final : public FilterBase {
public:
    static constexpr std::string_view kName = "Color map";
    static constexpr std::size_t kMapParameter = 0;

    ColorMapFilter();
    cv::Mat apply(const cv::Mat& input) const override;
};

}  // namespace coral