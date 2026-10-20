#pragma once
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace coral::testing {

inline constexpr int kBoardImageSize = 324;

inline std::vector<std::uint8_t> encodeJpeg(const cv::Mat& image) {
    std::vector<std::uint8_t> bytes;
    cv::imencode(".jpg", image, bytes);
    return bytes;
}

inline std::vector<std::uint8_t> boardSizedJpeg() {
    cv::Mat image(kBoardImageSize, kBoardImageSize, CV_8UC3, cv::Scalar(40, 120, 200));
    return encodeJpeg(image);
}

inline std::string toBase64(const std::vector<std::uint8_t>& bytes) {
    static constexpr char kAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    std::size_t i = 0;
    for (; i + 2 < bytes.size(); i += 3) {
        const std::uint32_t v = (bytes[i] << 16) | (bytes[i + 1] << 8) | bytes[i + 2];
        out += kAlphabet[(v >> 18) & 63];
        out += kAlphabet[(v >> 12) & 63];
        out += kAlphabet[(v >> 6) & 63];
        out += kAlphabet[v & 63];
    }
    if (i < bytes.size()) {
        std::uint32_t v = bytes[i] << 16;
        const bool hasSecond = i + 1 < bytes.size();
        if (hasSecond) v |= bytes[i + 1] << 8;
        out += kAlphabet[(v >> 18) & 63];
        out += kAlphabet[(v >> 12) & 63];
        out += hasSecond ? kAlphabet[(v >> 6) & 63] : '=';
        out += '=';
    }
    return out;
}

inline std::string frameLine(const std::vector<std::uint8_t>& jpeg) {
    return "FRAME:" + toBase64(jpeg) + "\r\n";
}

}  // namespace coral::testing