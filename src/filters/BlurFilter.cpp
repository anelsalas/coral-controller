#include "filters/BlurFilter.h"

#include <opencv2/imgproc.hpp>

namespace coral {
namespace {
constexpr int kDefaultKernelSize = 5;
constexpr int kMaxKernelSize = 31;
constexpr double kMaxSigma = 10.0;
constexpr double kAutoSigma = 0.0;  // OpenCV derives sigma from the kernel size
}  // namespace

BlurFilter::BlurFilter()
    : FilterBase(std::string(kName), {FilterParameter::integer("Kernel size", kDefaultKernelSize, 1, kMaxKernelSize),
                                      FilterParameter::real("Sigma (0 = auto)", kAutoSigma, 0.0, kMaxSigma)}) {}

cv::Mat BlurFilter::apply(const cv::Mat& input) const {
    const int oddKernelSize = parameter(kKernelSizeParameter).asInt() | 1;  // Gaussian kernels must be odd
    cv::Mat output;
    cv::GaussianBlur(input, output, cv::Size(oddKernelSize, oddKernelSize), parameter(kSigmaParameter).asReal());
    return output;
}

}  // namespace coral