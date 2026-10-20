// VIEW-011: snapshots to timestamped PNG files.
#include "imaging/SnapshotWriter.h"

#include <gtest/gtest.h>
#include <opencv2/imgcodecs.hpp>

#include <filesystem>
#include <fstream>

using namespace coral;
namespace fs = std::filesystem;

namespace {
fs::path freshTempFolder(const std::string& name) {
    const fs::path folder = fs::temp_directory_path() / "coral_controller_tests" / name;
    fs::remove_all(folder);
    return folder;
}
}  // namespace

TEST(SnapshotWriter, WritesTimestampedPng) {
    const fs::path folder = freshTempFolder("snapshots");
    SnapshotWriter writer(folder);
    const cv::Mat image(30, 40, CV_8UC3, cv::Scalar(10, 20, 30));

    const auto result = writer.write(image, std::chrono::system_clock::now(), "processed");

    ASSERT_TRUE(result.ok()) << result.error();
    const fs::path written = result.value();
    EXPECT_EQ(written.parent_path(), folder);
    EXPECT_EQ(written.extension(), ".png");
    EXPECT_EQ(written.filename().string().rfind("snapshot_", 0), 0u);
    const cv::Mat reloaded = cv::imread(written.string());
    ASSERT_FALSE(reloaded.empty());
    EXPECT_EQ(reloaded.size(), image.size());
}

TEST(SnapshotWriter, ReportsWriteFailure) {
    const fs::path folder = freshTempFolder("blocked");
    fs::create_directories(folder.parent_path());
    std::ofstream(folder.string()) << "a file where the folder should be";
    SnapshotWriter writer(folder);

    const auto result = writer.write(cv::Mat(4, 4, CV_8UC3), std::chrono::system_clock::now(), "processed");

    EXPECT_FALSE(result.ok());
    EXPECT_FALSE(result.error().empty());
    fs::remove(folder);
}

TEST(SnapshotWriter, RejectsEmptyImage) {
    SnapshotWriter writer(freshTempFolder("empty"));
    EXPECT_FALSE(writer.write(cv::Mat(), std::chrono::system_clock::now(), "raw").ok());
}