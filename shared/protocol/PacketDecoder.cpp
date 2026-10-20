#include "PacketDecoder.h"

#include "Crc32.h"

#include <cstring>

namespace coral {
namespace protocol {

PacketDecoder::PacketDecoder(IPacketListener& listener, std::uint8_t* storage, std::size_t capacity)
    : listener_(listener), storage_(storage), capacity_(capacity) {}

const DecoderCounters& PacketDecoder::counters() const { return counters_; }

void PacketDecoder::reset() { size_ = 0; }

std::size_t PacketDecoder::maxPayload() const {
    const std::size_t overhead = kHeaderBytes + kCrcBytes;
    const std::size_t byStorage = capacity_ > overhead ? capacity_ - overhead : 0;
    return byStorage < kMaxPayloadBytes ? byStorage : kMaxPayloadBytes;
}

void PacketDecoder::feed(const std::uint8_t* data, std::size_t length) {
    while (length > 0) {
        const std::size_t space = capacity_ - size_;
        const std::size_t chunk = length < space ? length : space;
        std::memcpy(storage_ + size_, data, chunk);
        size_ += chunk;
        data += chunk;
        length -= chunk;
        decodeBuffered();
        if (chunk == 0 && size_ == capacity_) drop(1);  // cannot happen with valid limits; guarantees progress
    }
}

void PacketDecoder::decodeBuffered() {
    while (alignToMagicCandidate() && size_ >= kHeaderBytes) {
        PacketHeader header;
        readHeader(storage_, &header);
        if (header.version != kProtocolVersion) {
            reject(RejectReason::kUnsupportedVersion, header);
            continue;
        }
        if (header.payloadLength > maxPayload()) {
            reject(RejectReason::kPayloadTooLarge, header);
            continue;
        }
        const std::size_t packetBytes = kHeaderBytes + header.payloadLength + kCrcBytes;
        if (size_ < packetBytes) return;  // wait for the rest
        const std::size_t crcOffset = kHeaderBytes + header.payloadLength;
        if (crc32(storage_, crcOffset) != getLe32(storage_ + crcOffset)) {
            reject(RejectReason::kBadCrc, header);
            continue;
        }
        ++counters_.packets;
        listener_.onPacket(header, storage_ + kHeaderBytes, header.payloadLength);
        drop(packetBytes);
    }
}

// Drops bytes until the buffer starts with kMagic (or a prefix of it at the end of the data).
// Returns false when nothing is left to decode.
bool PacketDecoder::alignToMagicCandidate() {
    std::size_t start = 0;
    for (; start < size_; ++start) {
        const std::size_t available = size_ - start;
        const std::size_t compare = available < kMagicBytes ? available : kMagicBytes;
        if (std::memcmp(storage_ + start, kMagic, compare) == 0) break;
    }
    counters_.discardedBytes += start;
    drop(start);
    return size_ > 0;
}

void PacketDecoder::reject(RejectReason reason, const PacketHeader& header) {
    switch (reason) {
        case RejectReason::kBadCrc: ++counters_.badCrc; break;
        case RejectReason::kUnsupportedVersion: ++counters_.unsupportedVersion; break;
        case RejectReason::kPayloadTooLarge: ++counters_.payloadTooLarge; break;
    }
    listener_.onReject(reason, header);
    drop(1);  // resync: search for the next magic after this false start
}

void PacketDecoder::drop(std::size_t count) {
    if (count == 0) return;
    if (count >= size_) {
        size_ = 0;
        return;
    }
    std::memmove(storage_, storage_ + count, size_ - count);
    size_ -= count;
}

}  // namespace protocol
}  // namespace coral