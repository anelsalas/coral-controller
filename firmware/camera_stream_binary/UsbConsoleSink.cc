#include "UsbConsoleSink.h"

#include "libs/base/console_m7.h"
#include "third_party/nxp/rt1176-sdk/devices/MIMXRT1176/utilities/debug_console/fsl_debug_console.h"

namespace coral {
namespace firmware {

void UsbConsoleSink::disableUartMirror() {
    // ConsoleM7 sends every message to the debug UART first (blocking, 115200 baud) and then to
    // USB CDC. With the debug console de-initialized, the UART send returns immediately
    // (g_serialHandle == NULL) and only USB CDC remains.
    DbgConsole_Deinit();
}

bool UsbConsoleSink::write(const std::uint8_t* data, std::size_t length) {
    auto* console = coralmicro::ConsoleM7::GetSingleton();
    while (length > 0) {
        const std::size_t chunk = length < kMaxChunkBytes ? length : kMaxChunkBytes;
        // ConsoleM7::Write copies the bytes into its queue; it cannot report USB errors.
        console->Write(reinterpret_cast<char*>(const_cast<std::uint8_t*>(data)), static_cast<int>(chunk));
        data += chunk;
        length -= chunk;
    }
    return true;
}

}  // namespace firmware
}  // namespace coral