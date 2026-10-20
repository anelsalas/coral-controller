#pragma once
#include "CoralProtocol.h"

namespace coral {
namespace protocol {

enum class RejectReason { kBadCrc, kUnsupportedVersion, kPayloadTooLarge };

class IPacketListener {
public:
    virtual ~IPacketListener() = default;
    // payload is valid only during the call.
    virtual void onPacket(const PacketHeader& header, const std::uint8_t* payload, std::size_t length) = 0;
    virtual void onReject(RejectReason reason, const PacketHeader& header) = 0;
};

struct DecoderCounters {
    std::uint64_t packets = 0;
    std::uint64_t badCrc = 0;
    std::uint64_t unsupportedVersion = 0;
    std::uint64_t payloadTooLarge = 0;
    std::uint64_t discardedBytes = 0;  // bytes skipped while searching for kMagic
};

// Incremental stream decoder with resynchronization (PROTO-003/004). Uses caller-provided
// storage (no heap); storage must hold the largest packet to accept
// (kMaxPacketBytes for full-size frames). Larger packets are rejected as too large.
class PacketDecoder {
public:
    PacketDecoder(IPacketListener& listener, std::uint8_t* storage, std::size_t capacity);

    void feed(const std::uint8_t* data, std::size_t length);
    void reset();
    const DecoderCounters& counters() const;

private:
    void decodeBuffered();
    bool alignToMagicCandidate();
    void reject(RejectReason reason, const PacketHeader& header);
    void drop(std::size_t count);
    std::size_t maxPayload() const;

    IPacketListener& listener_;
    std::uint8_t* storage_;
    std::size_t capacity_;
    std::size_t size_ = 0;
    DecoderCounters counters_;
};

}  // namespace protocol
}  // namespace coral