#pragma once
#include "common/Result.h"

#include <opencv2/core.hpp>

#include <cstdint>
#include <span>

namespace coral {

class JpegDecoder {
public:
    Result<cv::Mat> decode(std::span<const std::uint8_t> jpeg) const;
};

}  // namespace coral