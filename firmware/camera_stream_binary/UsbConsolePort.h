#ifndef FIRMWARE_CAMERA_STREAM_BINARY_USB_CONSOLE_PORT_H_
#define FIRMWARE_CAMERA_STREAM_BINARY_USB_CONSOLE_PORT_H_

#include "transport/PacedChunkSink.h"

namespace coral {
namespace firmware {

// One chunk = one SDK CdcAcm transfer (at most 512 bytes) over the USB CDC-ACM console
// (the COM port on Windows). disableUartMirror() switches off the console's 115200-baud
// UART copy so only USB CDC remains.
class UsbConsolePort final : public transport::IChunkPort {
public:
    static constexpr transport::PacingConfig kPacing{512, 300};

    static void disableUartMirror();
    void send(const std::uint8_t* data, std::size_t length) override;
    std::uint64_t nowMicros() const override;
    void waitUntilMicros(std::uint64_t deadline) override;
};

}  // namespace firmware
}  // namespace coral

#endif  // FIRMWARE_CAMERA_STREAM_BINARY_USB_CONSOLE_PORT_H_