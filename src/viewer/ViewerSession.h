#pragma once
#include "imaging/JpegDecoder.h"
#include "imaging/LatestFrameBuffer.h"
#include "protocol/PacketDecoder.h"
#include "protocol/ProtocolDetector.h"
#include "protocol/SequenceTracker.h"
#include "protocol/TextFrameParser.h"
#include "status/StatusModel.h"

namespace coral {

// Reader-thread side of the viewer: bytes -> (text | binary) protocol -> JPEG -> latest frame.
// Also the connection listener, so every new connection re-detects the protocol (PROTO-009).
// onConnected runs before the reader thread starts and onDisconnected after it stopped.
class ViewerSession final : public ITextProtocolListener,
                            public protocol::IPacketListener,
                            public IConnectionListener {
public:
    ViewerSession(LatestFrameBuffer& frames, StatusModel& status, const IClock& clock);

    void onBytes(std::span<const std::uint8_t> bytes);

    // IConnectionListener
    void onConnected(const SerialPortInfo& port) override;
    void onDisconnected(const std::string& reason) override;
    void onConnectError(const std::string& message) override;

    // ITextProtocolListener
    void onFrame(std::vector<std::uint8_t> jpeg) override;
    void onLogLine(const std::string& line) override;
    void onParseError(ParseErrorKind kind, const std::string& detail) override;

    // protocol::IPacketListener
    void onPacket(const protocol::PacketHeader& header, const std::uint8_t* payload, std::size_t length) override;
    void onReject(protocol::RejectReason reason, const protocol::PacketHeader& header) override;

private:
    void route(std::span<const std::uint8_t> bytes);
    void handleHello(const std::uint8_t* payload, std::size_t length);
    void handleFrame(const std::uint8_t* payload, std::size_t length);
    void publishJpeg(std::span<const std::uint8_t> jpeg);

    LatestFrameBuffer& frames_;
    StatusModel& status_;
    const IClock& clock_;
    ProtocolDetector detector_;
    TextFrameParser textParser_;
    std::vector<std::uint8_t> packetStorage_;
    protocol::PacketDecoder packetDecoder_;
    protocol::SequenceTracker sequences_;
    JpegDecoder decoder_;
    std::uint64_t nextFrameNumber_ = 1;
};

}  // namespace coral