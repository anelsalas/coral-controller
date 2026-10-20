#include "UsbConsolePort.h"

#include "libs/base/console_m7.h"
#include "libs/base/timer.h"
#include "third_party/freertos_kernel/include/FreeRTOS.h"
#include "third_party/freertos_kernel/include/task.h"
#include "third_party/nxp/rt1176-sdk/devices/MIMXRT1176/utilities/debug_console/fsl_debug_console.h"

namespace coral {
namespace firmware {

void UsbConsolePort::disableUartMirror() {
    // ConsoleM7 sends every message to the debug UART first (blocking, 115200 baud) and then to
    // USB CDC. With the debug console de-initialized, the UART send returns immediately
    // (g_serialHandle == NULL) and only USB CDC remains.
    DbgConsole_Deinit();
}

void UsbConsolePort::send(const std::uint8_t* data, std::size_t length) {
    // ConsoleM7::Write queues a copy for the console tx task; it reports no USB result.
    coralmicro::ConsoleM7::GetSingleton()->Write(reinterpret_cast<char*>(const_cast<std::uint8_t*>(data)),
                                                 static_cast<int>(length));
}

std::uint64_t UsbConsolePort::nowMicros() const { return coralmicro::TimerMicros(); }

void UsbConsolePort::waitUntilMicros(std::uint64_t deadline) {
    // Gaps are sub-millisecond, below the RTOS tick; yield instead of sleeping.
    while (coralmicro::TimerMicros() < deadline) taskYIELD();
}

}  // namespace firmware
}  // namespace coral