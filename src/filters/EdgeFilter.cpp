#include "filters/EdgeFilter.h"

#include "filters/ImageConversions.h"

#include <opencv2/imgproc.hpp>

namespace coral {
namespace {
constexpr int kMaxThreshold = 255;
constexpr int kDefaultLowThreshold = 50;
constexpr int kDefaultHighThreshold = 150;
}  // namespace

EdgeFilter::EdgeFilter()
    : FilterBase(std::string(kName),
                 {FilterParameter::integer("Low threshold", kDefaultLowThreshold, 0, kMaxThreshold),
                  FilterParameter::integer("High threshold", kDefaultHighThreshold, 0, kMaxThreshold)}) {}

cv::Mat EdgeFilter::apply(const cv::Mat& input) const {
    cv::Mat edges;
    cv::Canny(toGray(input), edges, parameter(kLowThresholdParameter).asInt(),
              parameter(kHighThresholdParameter).asInt());
    return edges;
}

}  // namespace coral