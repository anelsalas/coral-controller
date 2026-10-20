#include "filters/ImageConversions.h"

#include <opencv2/imgproc.hpp>

#include <stdexcept>
#include <string>

namespace coral {

cv::Mat toGray(const cv::Mat& image) {
    switch (image.channels()) {
        case 1: return image;
        case 3: {
            cv::Mat gray;
            cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
            return gray;
        }
        case 4: {
            cv::Mat gray;
            cv::cvtColor(image, gray, cv::COLOR_BGRA2GRAY);
            return gray;
        }
        default: throw std::invalid_argument("unsupported channel count " + std::to_string(image.channels()));
    }
}

}  // namespace coral