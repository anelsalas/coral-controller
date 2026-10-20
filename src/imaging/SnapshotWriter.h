#pragma once
#include "common/Result.h"

#include <opencv2/core.hpp>

#include <chrono>
#include <filesystem>
#include <string_view>

namespace coral {

// Writes PNG snapshots named snapshot_YYYYMMDD_HHMMSS_mmm_<label>.png (VIEW-011).
class SnapshotWriter {
public:
    explicit SnapshotWriter(std::filesystem::path folder);

    Result<std::filesystem::path> write(const cv::Mat& image, std::chrono::system_clock::time_point takenAt,
                                        std::string_view label) const;
    const std::filesystem::path& folder() const;

    static std::string fileNameFor(std::chrono::system_clock::time_point takenAt, std::string_view label);

private:
    std::filesystem::path folder_;
};

}  // namespace coral