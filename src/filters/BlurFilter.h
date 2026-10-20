#pragma once
#include "filters/FilterBase.h"

namespace coral {

class BlurFilter final : public FilterBase {
public:
    static constexpr std::string_view kName = "Gaussian blur";
    static constexpr std::size_t kKernelSizeParameter = 0;
    static constexpr std::size_t kSigmaParameter = 1;

    BlurFilter();
    cv::Mat apply(const cv::Mat& input) const override;
};

}  // namespace coral