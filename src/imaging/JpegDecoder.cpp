#include "imaging/JpegDecoder.h"

#include <opencv2/imgcodecs.hpp>

namespace coral {

Result<cv::Mat> JpegDecoder::decode(std::span<const std::uint8_t> jpeg) const {
    if (jpeg.empty()) return Result<cv::Mat>::failure("empty JPEG payload");
    try {
        const cv::Mat encoded(1, static_cast<int>(jpeg.size()), CV_8UC1, const_cast<std::uint8_t*>(jpeg.data()));
        cv::Mat image = cv::imdecode(encoded, cv::IMREAD_COLOR);
        if (image.empty()) {
            return Result<cv::Mat>::failure("undecodable JPEG (" + std::to_string(jpeg.size()) + " bytes)");
        }
        return Result<cv::Mat>::success(std::move(image));
    } catch (const cv::Exception& error) {
        return Result<cv::Mat>::failure(std::string("JPEG decode error: ") + error.what());
    }
}

}  // namespace coral