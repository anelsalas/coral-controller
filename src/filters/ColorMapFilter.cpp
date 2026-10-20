#include "filters/ColorMapFilter.h"

#include "filters/ImageConversions.h"

#include <opencv2/imgproc.hpp>

namespace coral {

ColorMapFilter::ColorMapFilter()
    : FilterBase(std::string(kName), {FilterParameter::choice("Map", {"Jet", "Turbo", "Viridis", "Inferno", "Hot"}, 1)}) {}

cv::Mat ColorMapFilter::apply(const cv::Mat& input) const {
    static constexpr cv::ColormapTypes kMaps[] = {cv::COLORMAP_JET, cv::COLORMAP_TURBO, cv::COLORMAP_VIRIDIS,
                                                  cv::COLORMAP_INFERNO, cv::COLORMAP_HOT};
    cv::Mat output;
    cv::applyColorMap(toGray(input), output, kMaps[parameter(kMapParameter).asChoiceIndex()]);
    return output;
}

}  // namespace coral