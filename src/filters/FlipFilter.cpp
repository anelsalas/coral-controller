#include "filters/FlipFilter.h"

namespace coral {

FlipFilter::FlipFilter()
    : FilterBase(std::string(kName),
                 {FilterParameter::choice("Direction", {"Horizontal", "Vertical", "Both"}, kHorizontal)}) {}

cv::Mat FlipFilter::apply(const cv::Mat& input) const {
    // OpenCV flip codes: 1 = around y axis (horizontal), 0 = around x axis (vertical), -1 = both.
    static constexpr int kFlipCodes[] = {1, 0, -1};
    cv::Mat output;
    cv::flip(input, output, kFlipCodes[parameter(kDirectionParameter).asChoiceIndex()]);
    return output;
}

}  // namespace coral