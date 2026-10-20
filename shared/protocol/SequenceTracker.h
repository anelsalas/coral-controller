#pragma once
#include <cstdint>

namespace coral {
namespace protocol {

// Counts packets lost between consecutive sequence numbers (PROTO-005). A jump backwards
// (board restarted) restarts tracking instead of counting ~4 billion drops.
class SequenceTracker {
public:
    void observe(std::uint32_t sequence);
    void restart();
    std::uint64_t droppedPackets() const;

private:
    static constexpr std::uint32_t kMaxForwardGap = 0x7FFFFFFFu;

    bool hasPrevious_ = false;
    std::uint32_t expected_ = 0;
    std::uint64_t dropped_ = 0;
};

}  // namespace protocol
}  // namespace coral