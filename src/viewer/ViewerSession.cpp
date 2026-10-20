#include "viewer/ViewerSession.h"

#include "protocol/Payloads.h"

namespace coral {

ViewerSession::ViewerSession(LatestFrameBuffer& frames, StatusModel& status, const IClock& clock)
    : frames_(frames),
      status_(status),
      clock_(clock),
      textParser_(*this),
      packetStorage_(protocol::kMaxPacketBytes),
      packetDecoder_(*this, packetStorage_.data(), packetStorage_.size()) {}

void ViewerSession::onConnected(const SerialPortInfo& port) {
    detector_.reset();
    textParser_.reset();
    packetDecoder_.reset();
    sequences_.restart();
    status_.onConnected(port);
}

void ViewerSession::onDisconnected(const std::string& reason) { status_.onDisconnected(reason); }
void ViewerSession::onConnectError(const std::string& message) { status_.onConnectError(message); }

void ViewerSession::onBytes(std::span<const std::uint8_t> bytes) {
    if (detector_.protocol() != StreamProtocol::Unknown) {
        route(bytes);
        return;
    }
    const StreamProtocol detected = detector_.feed(bytes);
    if (detected == StreamProtocol::Unknown) return;
    status_.onProtocolDetected(detected == StreamProtocol::Binary ? StatusModel::kBinaryProtocolName
                                                                  : StatusModel::kTextProtocolName);
    route(detector_.takeBufferedBytes());
}

void ViewerSession::route(std::span<const std::uint8_t> bytes) {
    if (detector_.protocol() == StreamProtocol::Binary) {
        packetDecoder_.feed(bytes.data(), bytes.size());
    } else {
        textParser_.feed(bytes);
    }
}

void ViewerSession::onFrame(std::vector<std::uint8_t> jpeg) { publishJpeg(jpeg); }

void ViewerSession::publishJpeg(std::span<const std::uint8_t> jpeg) {
    auto decoded = decoder_.decode(jpeg);
    if (!decoded.ok()) {
        status_.onDecodeError(decoded.error());
        return;
    }
    const TimePoint now = clock_.now();
    status_.onFrameDecoded(now);
    frames_.publish(Frame{std::move(decoded.value()), now, nextFrameNumber_++});
}

void ViewerSession::onLogLine(const std::string& line) { status_.onLogLine(line); }

void ViewerSession::onParseError(ParseErrorKind kind, const std::string& detail) {
    status_.onParseError(kind, detail);
}

void ViewerSession::onPacket(const protocol::PacketHeader& header, const std::uint8_t* payload, std::size_t length) {
    if (header.type == protocol::PacketType::kHello) sequences_.restart();  // board (re)started its sequence
    sequences_.observe(header.sequence);
    status_.onDroppedPackets(sequences_.droppedPackets());
    switch (header.type) {
        case protocol::PacketType::kHello: handleHello(payload, length); return;
        case protocol::PacketType::kFrame: handleFrame(payload, length); return;
        case protocol::PacketType::kLog:
            status_.onLogLine(std::string(reinterpret_cast<const char*>(payload), length));
            return;
    }
    status_.onPacketRejected("unknown packet type " + std::to_string(static_cast<int>(header.type)));
}

void ViewerSession::handleHello(const std::uint8_t* payload, std::size_t length) {
    protocol::HelloPayload hello;
    if (!protocol::decodeHello(payload, length, &hello)) {
        status_.onPacketRejected("malformed HELLO payload (" + std::to_string(length) + " bytes)");
        return;
    }
    status_.onBoardIdentity(hello.appName, hello.appVersion);
}

void ViewerSession::handleFrame(const std::uint8_t* payload, std::size_t length) {
    protocol::FrameInfo info;
    if (!protocol::decodeFrameInfo(payload, length, &info)) {
        status_.onPacketRejected("malformed FRAME payload (" + std::to_string(length) + " bytes)");
        return;
    }
    publishJpeg(std::span<const std::uint8_t>(payload + protocol::kFrameInfoBytes, length - protocol::kFrameInfoBytes));
}

void ViewerSession::onReject(protocol::RejectReason reason, const protocol::PacketHeader& header) {
    switch (reason) {
        case protocol::RejectReason::kUnsupportedVersion: status_.onIncompatibleProtocol(header.version); return;
        case protocol::RejectReason::kBadCrc: status_.onPacketRejected("packet CRC mismatch"); return;
        case protocol::RejectReason::kPayloadTooLarge:
            status_.onPacketRejected("packet length " + std::to_string(header.payloadLength) + " exceeds maximum");
            return;
    }
}

}  // namespace coral