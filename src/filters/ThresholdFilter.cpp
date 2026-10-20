#include "filters/ThresholdFilter.h"

#include "filters/ImageConversions.h"

#include <opencv2/imgproc.hpp>

namespace coral {
namespace {
constexpr int kDefaultLevel = 128;
constexpr int kMaxLevel = 255;
constexpr double kForegroundValue = 255.0;
}  // namespace

ThresholdFilter::ThresholdFilter()
    : FilterBase(std::string(kName), {FilterParameter::integer("Level", kDefaultLevel, 0, kMaxLevel),
                                      FilterParameter::boolean("Invert", false)}) {}

cv::Mat ThresholdFilter::apply(const cv::Mat& input) const {
    const int mode = parameter(kInvertParameter).asBool() ? cv::THRESH_BINARY_INV : cv::THRESH_BINARY;
    cv::Mat output;
    cv::threshold(toGray(input), output, parameter(kLevelParameter).asInt(), kForegroundValue, mode);
    return output;
}

}  // namespace coral