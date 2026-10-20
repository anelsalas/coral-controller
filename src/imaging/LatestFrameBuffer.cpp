#include "imaging/LatestFrameBuffer.h"

namespace coral {

void LatestFrameBuffer::publish(Frame frame) {
    std::lock_guard lock(mutex_);
    if (latest_) ++dropped_;
    latest_ = std::move(frame);
}

std::optional<Frame> LatestFrameBuffer::takeLatest() {
    std::lock_guard lock(mutex_);
    std::optional<Frame> taken;
    taken.swap(latest_);
    return taken;
}

std::uint64_t LatestFrameBuffer::droppedCount() const {
    std::lock_guard lock(mutex_);
    return dropped_;
}

}  // namespace coral