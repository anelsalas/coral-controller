#include "transport/PacedChunkSink.h"

namespace coral {
namespace transport {

PacedChunkSink::PacedChunkSink(IChunkPort& port, PacingConfig config) : port_(port), config_(config) {}

bool PacedChunkSink::write(const std::uint8_t*, std::size_t) { return true; }

void PacedChunkSink::sendPaced(const std::uint8_t*, std::size_t) {}

}  // namespace transport
}  // namespace coral
