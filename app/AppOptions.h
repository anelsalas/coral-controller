#pragma once
#include "common/Result.h"

#include <filesystem>
#include <optional>

namespace coral::app {

struct AppOptions {
    std::filesystem::path snapshotFolder = "snapshots";
    // Smoke test: save snapshots after this many displayed frames, then exit.
    std::optional<int> snapshotAfterFrames;
};

// Usage: CoralController [--snapshot-dir <folder>] [--snapshot-after-frames <n>]
Result<AppOptions> parseOptions(int argc, char** argv);

}  // namespace coral::app