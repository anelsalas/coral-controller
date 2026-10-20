#pragma once
#include "common/Clock.h"

#include <opencv2/core.hpp>

#include <cstdint>

namespace coral {

struct Frame {
    cv::Mat image;  // BGR as decoded by OpenCV
    TimePoint receivedAt{};
    std::uint64_t sequence = 0;
};

}  // namespace coral