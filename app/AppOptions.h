#pragma once
#include "common/Result.h"

#include <filesystem>
#include <optional>

namespace coral::app {

struct AppOptions {
    std::filesystem::path snapshotFolder = "snapshots";
    // Smoke test: save snapshots after this many displayed frames, then exit.
    std::optional<int> snapshotAfterFrames;
    // Measurement: after the first frame, count decoded frames for this many seconds, print fps, exit.
    std::optional<int> measureSeconds;
};

// Usage: CoralController [--snapshot-dir <folder>] [--snapshot-after-frames <n>] [--measure-fps <seconds>]
Result<AppOptions> parseOptions(int argc, char** argv);

}  // namespace coral::app