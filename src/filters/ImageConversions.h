#pragma once
#include <opencv2/core.hpp>

namespace coral {

// 1, 3 (BGR) or 4 (BGRA) channels -> 1 channel. Throws std::invalid_argument otherwise.
cv::Mat toGray(const cv::Mat& image);

}  // namespace coral