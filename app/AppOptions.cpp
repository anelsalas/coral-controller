#include "AppOptions.h"

#include <charconv>
#include <string_view>

namespace coral::app {

Result<AppOptions> parseOptions(int argc, char** argv) {
    AppOptions options;
    for (int i = 1; i < argc; ++i) {
        const std::string_view argument = argv[i];
        const bool hasValue = i + 1 < argc;
        if (argument == "--snapshot-dir" && hasValue) {
            options.snapshotFolder = argv[++i];
        } else if (argument == "--snapshot-after-frames" && hasValue) {
            const std::string_view text = argv[++i];
            int frames = 0;
            const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), frames);
            if (error != std::errc{} || end != text.data() + text.size() || frames < 1) {
                return Result<AppOptions>::failure("--snapshot-after-frames needs a positive number");
            }
            options.snapshotAfterFrames = frames;
        } else {
            return Result<AppOptions>::failure("unknown or incomplete argument: " + std::string(argument));
        }
    }
    return Result<AppOptions>::success(options);
}

}  // namespace coral::app