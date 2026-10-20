#ifndef FIRMWARE_CAMERA_STREAM_BINARY_USB_CONSOLE_SINK_H_
#define FIRMWARE_CAMERA_STREAM_BINARY_USB_CONSOLE_SINK_H_

#include "protocol/PacketWriter.h"

namespace coral {
namespace firmware {

// Sends bytes over the board's USB CDC-ACM console (the COM port on Windows).
// The SDK's CdcAcm transmits at most 512 bytes per transfer, so writes are chunked.
// The UART mirror of the console is switched off by disableUartMirror() so the
// 115200-baud debug UART no longer throttles throughput.
class UsbConsoleSink final : public protocol::IByteSink {
public:
    static constexpr std::size_t kMaxChunkBytes = 512;

    static void disableUartMirror();
    bool write(const std::uint8_t* data, std::size_t length) override;
};

}  // namespace firmware
}  // namespace coral

#endif  // FIRMWARE_CAMERA_STREAM_BINARY_USB_CONSOLE_SINK_H_