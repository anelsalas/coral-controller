#include "AppOptions.h"

#include <charconv>
#include <string_view>

namespace coral::app {

namespace {
std::optional<int> parsePositive(std::string_view text) {
    int value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size() || value < 1) return std::nullopt;
    return value;
}
}  // namespace

Result<AppOptions> parseOptions(int argc, char** argv) {
    AppOptions options;
    for (int i = 1; i < argc; ++i) {
        const std::string_view argument = argv[i];
        const bool hasValue = i + 1 < argc;
        if (argument == "--snapshot-dir" && hasValue) {
            options.snapshotFolder = argv[++i];
        } else if (argument == "--snapshot-after-frames" && hasValue) {
            options.snapshotAfterFrames = parsePositive(argv[++i]);
            if (!options.snapshotAfterFrames) {
                return Result<AppOptions>::failure("--snapshot-after-frames needs a positive number");
            }
        } else if (argument == "--measure-fps" && hasValue) {
            options.measureSeconds = parsePositive(argv[++i]);
            if (!options.measureSeconds) return Result<AppOptions>::failure("--measure-fps needs positive seconds");
        } else {
            return Result<AppOptions>::failure("unknown or incomplete argument: " + std::string(argument));
        }
    }
    return Result<AppOptions>::success(options);
}

}  // namespace coral::app