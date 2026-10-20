#pragma once
#include "protocol/PacketWriter.h"

#include <cstddef>
#include <cstdint>

namespace coral {
namespace transport {

// One USB transfer at a time, plus a clock. Firmware: ConsoleM7 + TimerMicros; tests: a fake.
class IChunkPort {
public:
    virtual ~IChunkPort() = default;
    virtual void send(const std::uint8_t* data, std::size_t length) = 0;
    virtual std::uint64_t nowMicros() const = 0;
    virtual void waitUntilMicros(std::uint64_t deadline) = 0;
};

struct PacingConfig {
    std::size_t maxChunkBytes;
    std::uint64_t minGapMicros;  // minimum time between the starts of two chunks
};

// Splits bytes into chunks and keeps at least minGapMicros between chunk starts.
//
// Why: after a host closes the port mid-transfer, the coralmicro SDK's CdcAcm::Transmit
// can be left one semaphore "give" ahead. From then on Transmit returns before its transfer
// has finished, and the next chunk overwrites the SDK's single tx buffer while it is still on
// the wire. Leaving the transfer time to finish before the next chunk keeps the data intact.
class PacedChunkSink final : public protocol::IByteSink {
public:
    PacedChunkSink(IChunkPort& port, PacingConfig config);

    // Always true: the SDK console reports no transmit result to the application.
    bool write(const std::uint8_t* data, std::size_t length) override;

private:
    void sendPaced(const std::uint8_t* data, std::size_t length);

    IChunkPort& port_;
    PacingConfig config_;
    bool hasSent_ = false;
    std::uint64_t lastSendMicros_ = 0;
};

}  // namespace transport
}  // namespace coral