// camera_stream_binary: streams camera frames as Coral Controller binary packets
// ("CRLP" v1, see shared/protocol/CoralProtocol.h) over USB CDC, without base64 (PROTO-007).
// Announces app name/version at startup and periodically (PROTO-006).
// Embedded rules (PROTO-008): no exceptions, all buffers static with fixed sizes.
#include <cstdint>

#include "UsbConsoleSink.h"
#include "libs/base/led.h"
#include "libs/camera/camera.h"
#include "libs/libjpeg/jpeg.h"
#include "protocol/PacketWriter.h"
#include "protocol/Payloads.h"
#include "third_party/freertos_kernel/include/FreeRTOS.h"
#include "third_party/freertos_kernel/include/task.h"

namespace coral {
namespace firmware {
namespace {

using coralmicro::CameraTask;

constexpr char kAppName[] = "camera_stream_binary";
constexpr char kAppVersion[] = CAMERA_STREAM_BINARY_VERSION;
constexpr int kJpegQuality = 75;
constexpr std::uint32_t kHelloEveryFrames = 30;  // late-connecting PCs still learn the app identity
constexpr TickType_t kCameraRetryDelay = pdMS_TO_TICKS(10);
constexpr std::size_t kRgbBytes = CameraTask::kWidth * CameraTask::kHeight * 3;
constexpr std::size_t kMaxJpegBytes = protocol::kMaxPayloadBytes - protocol::kFrameInfoBytes;

#define CORAL_SDRAM_BSS __attribute__((section(".sdram_bss,\"aw\",%nobits @")))
std::uint8_t g_rgb[kRgbBytes] __attribute__((aligned(16))) CORAL_SDRAM_BSS;
std::uint8_t g_jpeg[kMaxJpegBytes] __attribute__((aligned(16))) CORAL_SDRAM_BSS;

void sendHello(protocol::PacketWriter& writer) {
    std::uint8_t payload[protocol::kHelloPayloadBytes];
    protocol::encodeHello(kAppName, kAppVersion, payload);
    writer.write(protocol::PacketType::kHello, payload, sizeof(payload));
}

void sendLog(protocol::PacketWriter& writer, const char* text, std::size_t length) {
    writer.write(protocol::PacketType::kLog, reinterpret_cast<const std::uint8_t*>(text), length);
}

bool startCamera() {
    return CameraTask::GetSingleton()->SetPower(true) &&
           CameraTask::GetSingleton()->Enable(coralmicro::CameraMode::kStreaming);
}

bool captureRgb() {
    coralmicro::CameraFrameFormat format{coralmicro::CameraFormat::kRgb,
                                         coralmicro::CameraFilterMethod::kBilinear,
                                         coralmicro::CameraRotation::k0,
                                         static_cast<int>(CameraTask::kWidth),
                                         static_cast<int>(CameraTask::kHeight),
                                         false,
                                         g_rgb,
                                         true};
    return CameraTask::GetSingleton()->GetFrame({format});
}

// Returns the JPEG size, 0 on failure.
std::size_t compressJpeg() {
    return coralmicro::JpegCompressRgb(g_rgb, CameraTask::kWidth, CameraTask::kHeight, kJpegQuality, g_jpeg,
                                       kMaxJpegBytes);
}

void sendFrame(protocol::PacketWriter& writer, std::size_t jpegBytes) {
    std::uint8_t info[protocol::kFrameInfoBytes];
    protocol::encodeFrameInfo(protocol::FrameInfo{static_cast<std::uint16_t>(CameraTask::kWidth),
                                                  static_cast<std::uint16_t>(CameraTask::kHeight),
                                                  protocol::ImageFormat::kJpeg},
                              info);
    writer.write(protocol::PacketType::kFrame, info, sizeof(info), g_jpeg, jpegBytes);
}

[[noreturn]] void streamFrames(protocol::PacketWriter& writer) {
    static constexpr char kCaptureFailed[] = "camera: unable to get frame";
    static constexpr char kJpegFailed[] = "jpeg: compression failed or exceeded buffer";
    std::uint32_t frames = 0;
    while (true) {
        if (!captureRgb()) {
            sendLog(writer, kCaptureFailed, sizeof(kCaptureFailed) - 1);
            vTaskDelay(kCameraRetryDelay);
            continue;
        }
        const std::size_t jpegBytes = compressJpeg();
        if (jpegBytes == 0) {
            sendLog(writer, kJpegFailed, sizeof(kJpegFailed) - 1);
            continue;
        }
        sendFrame(writer, jpegBytes);
        if (++frames % kHelloEveryFrames == 0) sendHello(writer);
    }
}

void run() {
    UsbConsoleSink::disableUartMirror();
    UsbConsoleSink sink;
    protocol::PacketWriter writer(sink);
    coralmicro::LedSet(coralmicro::Led::kStatus, true);
    sendHello(writer);
    if (!startCamera()) {
        static constexpr char kCameraFailed[] = "camera: power/enable failed";
        while (true) {
            sendLog(writer, kCameraFailed, sizeof(kCameraFailed) - 1);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    streamFrames(writer);
}

}  // namespace
}  // namespace firmware
}  // namespace coral

extern "C" void app_main(void* param) {
    (void)param;
    coral::firmware::run();
}