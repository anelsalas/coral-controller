#pragma once
#include "CoralProtocol.h"

namespace coral {
namespace protocol {

// Destination for encoded bytes (USB CDC on the board, a buffer in tests).
class IByteSink {
public:
    virtual ~IByteSink() = default;
    virtual bool write(const std::uint8_t* data, std::size_t length) = 0;
};

enum class WriteStatus { kOk, kPayloadTooLarge, kSinkFailed };

// Frames payloads into packets without copying the payload (no heap). The payload may be
// given in two parts (e.g. FrameInfo + JPEG) so callers need no staging buffer.
class PacketWriter {
public:
    explicit PacketWriter(IByteSink& sink);

    WriteStatus write(PacketType type, const std::uint8_t* payload, std::size_t length);
    WriteStatus write(PacketType type, const std::uint8_t* first, std::size_t firstLength,
                      const std::uint8_t* second, std::size_t secondLength);
    std::uint32_t nextSequence() const;

private:
    IByteSink& sink_;
    std::uint32_t nextSequence_ = 0;
};

}  // namespace protocol
}  // namespace coral