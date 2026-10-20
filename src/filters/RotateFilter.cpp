#include "filters/RotateFilter.h"

#include <opencv2/core.hpp>

namespace coral {

RotateFilter::RotateFilter()
    : FilterBase(std::string(kName),
                 {FilterParameter::choice("Angle", {"90 clockwise", "180", "90 counter-clockwise"}, kClockwise90)}) {}

cv::Mat RotateFilter::apply(const cv::Mat& input) const {
    static constexpr cv::RotateFlags kRotations[] = {cv::ROTATE_90_CLOCKWISE, cv::ROTATE_180,
                                                     cv::ROTATE_90_COUNTERCLOCKWISE};
    cv::Mat output;
    cv::rotate(input, output, kRotations[parameter(kAngleParameter).asChoiceIndex()]);
    return output;
}

}  // namespace coral