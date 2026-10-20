#include "PacketWriter.h"

#include "Crc32.h"

namespace coral {
namespace protocol {

PacketWriter::PacketWriter(IByteSink& sink) : sink_(sink) {}

std::uint32_t PacketWriter::nextSequence() const { return nextSequence_; }

WriteStatus PacketWriter::write(PacketType type, const std::uint8_t* payload, std::size_t length) {
    return write(type, payload, length, nullptr, 0);
}

WriteStatus PacketWriter::write(PacketType type, const std::uint8_t* first, std::size_t firstLength,
                                const std::uint8_t* second, std::size_t secondLength) {
    const std::size_t payloadLength = firstLength + secondLength;
    if (payloadLength > kMaxPayloadBytes || payloadLength < firstLength) return WriteStatus::kPayloadTooLarge;

    PacketHeader header;
    header.type = type;
    header.sequence = nextSequence_;
    header.payloadLength = static_cast<std::uint32_t>(payloadLength);
    std::uint8_t headerBytes[kHeaderBytes];
    writeHeader(header, headerBytes);

    std::uint32_t crc = crc32(headerBytes, kHeaderBytes);
    if (firstLength > 0) crc = crc32(first, firstLength, crc);
    if (secondLength > 0) crc = crc32(second, secondLength, crc);
    std::uint8_t crcBytes[kCrcBytes];
    putLe32(crcBytes, crc);

    ++nextSequence_;  // consumed even on sink failure so the PC sees the gap
    const bool written = sink_.write(headerBytes, kHeaderBytes) &&
                         (firstLength == 0 || sink_.write(first, firstLength)) &&
                         (secondLength == 0 || sink_.write(second, secondLength)) &&
                         sink_.write(crcBytes, kCrcBytes);
    return written ? WriteStatus::kOk : WriteStatus::kSinkFailed;
}

}  // namespace protocol
}  // namespace coral