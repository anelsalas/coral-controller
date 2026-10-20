#include "transport/PacedChunkSink.h"

#include <gtest/gtest.h>

#include <numeric>
#include <vector>

using namespace coral::transport;

namespace {
struct Sent {
    std::uint64_t atMicros;
    std::vector<std::uint8_t> bytes;
};

class FakePort final : public IChunkPort {
public:
    void send(const std::uint8_t* data, std::size_t length) override {
        sent.push_back({now, std::vector<std::uint8_t>(data, data + length)});
    }
    std::uint64_t nowMicros() const override { return now; }
    void waitUntilMicros(std::uint64_t deadline) override {
        ++waits;
        if (deadline > now) now = deadline;
    }

    std::uint64_t now = 1000;
    int waits = 0;
    std::vector<Sent> sent;
};

constexpr PacingConfig kConfig{512, 300};

std::vector<std::uint8_t> bytes(std::size_t count) {
    std::vector<std::uint8_t> data(count);
    std::iota(data.begin(), data.end(), std::uint8_t{0});
    return data;
}
}  // namespace

TEST(PacedChunkSink, SplitsIntoChunksOfAtMostMaxBytesPreservingOrder) {
    FakePort port;
    PacedChunkSink sink(port, kConfig);
    const auto data = bytes(1300);

    EXPECT_TRUE(sink.write(data.data(), data.size()));

    ASSERT_EQ(port.sent.size(), 3u);
    EXPECT_EQ(port.sent[0].bytes.size(), 512u);
    EXPECT_EQ(port.sent[1].bytes.size(), 512u);
    EXPECT_EQ(port.sent[2].bytes.size(), 276u);
    std::vector<std::uint8_t> joined;
    for (const auto& chunk : port.sent) joined.insert(joined.end(), chunk.bytes.begin(), chunk.bytes.end());
    EXPECT_EQ(joined, data);
}

TEST(PacedChunkSink, FirstChunkIsSentWithoutWaiting) {
    FakePort port;
    PacedChunkSink sink(port, kConfig);
    const auto data = bytes(10);

    sink.write(data.data(), data.size());

    EXPECT_EQ(port.waits, 0);
    EXPECT_EQ(port.sent.at(0).atMicros, 1000u);
}

TEST(PacedChunkSink, KeepsMinimumGapBetweenChunkStarts) {
    FakePort port;
    PacedChunkSink sink(port, kConfig);
    const auto data = bytes(2048);

    sink.write(data.data(), data.size());

    ASSERT_EQ(port.sent.size(), 4u);
    for (std::size_t i = 1; i < port.sent.size(); ++i) {
        EXPECT_GE(port.sent[i].atMicros - port.sent[i - 1].atMicros, kConfig.minGapMicros) << "chunk " << i;
    }
}

TEST(PacedChunkSink, GapAppliesAcrossSeparateWrites) {
    FakePort port;
    PacedChunkSink sink(port, kConfig);
    const auto header = bytes(16);

    sink.write(header.data(), header.size());
    sink.write(header.data(), header.size());

    ASSERT_EQ(port.sent.size(), 2u);
    EXPECT_EQ(port.sent[1].atMicros - port.sent[0].atMicros, kConfig.minGapMicros);
}

TEST(PacedChunkSink, DoesNotWaitWhenTheGapHasAlreadyPassed) {
    FakePort port;
    PacedChunkSink sink(port, kConfig);
    const auto data = bytes(16);

    sink.write(data.data(), data.size());
    port.now += 5000;  // e.g. JPEG compression between frames
    sink.write(data.data(), data.size());

    EXPECT_EQ(port.waits, 0);
}

TEST(PacedChunkSink, EmptyWriteSendsNothing) {
    FakePort port;
    PacedChunkSink sink(port, kConfig);

    EXPECT_TRUE(sink.write(nullptr, 0));

    EXPECT_TRUE(port.sent.empty());
}