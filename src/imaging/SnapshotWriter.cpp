#include "imaging/SnapshotWriter.h"

#include <opencv2/imgcodecs.hpp>

#include <ctime>
#include <format>

namespace coral {

SnapshotWriter::SnapshotWriter(std::filesystem::path folder) : folder_(std::move(folder)) {}

const std::filesystem::path& SnapshotWriter::folder() const { return folder_; }

std::string SnapshotWriter::fileNameFor(std::chrono::system_clock::time_point takenAt, std::string_view label) {
    const std::time_t seconds = std::chrono::system_clock::to_time_t(takenAt);
    const auto millis =
        std::chrono::duration_cast<std::chrono::milliseconds>(takenAt.time_since_epoch()).count() % 1000;
    std::tm local{};
    localtime_s(&local, &seconds);
    return std::format("snapshot_{:04}{:02}{:02}_{:02}{:02}{:02}_{:03}_{}.png", local.tm_year + 1900,
                       local.tm_mon + 1, local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec, millis, label);
}

Result<std::filesystem::path> SnapshotWriter::write(const cv::Mat& image,
                                                    std::chrono::system_clock::time_point takenAt,
                                                    std::string_view label) const {
    using PathResult = Result<std::filesystem::path>;
    if (image.empty()) return PathResult::failure("no image to save");
    std::error_code error;
    std::filesystem::create_directories(folder_, error);
    if (error) return PathResult::failure("Cannot create folder " + folder_.string() + ": " + error.message());
    const std::filesystem::path path = folder_ / fileNameFor(takenAt, label);
    try {
        if (!cv::imwrite(path.string(), image)) return PathResult::failure("Cannot write " + path.string());
    } catch (const cv::Exception& exception) {
        return PathResult::failure("Cannot write " + path.string() + ": " + exception.what());
    }
    return PathResult::success(path);
}

}  // namespace coral