// PROTO-005: dropped-packet detection.
#include "protocol/SequenceTracker.h"

#include <gtest/gtest.h>

using coral::protocol::SequenceTracker;

TEST(SequenceTracker, CountsGaps) {
    SequenceTracker tracker;
    tracker.observe(1);
    tracker.observe(2);
    tracker.observe(5);
    EXPECT_EQ(tracker.droppedPackets(), 2u);
}

TEST(SequenceTracker, HandlesWrapAround) {
    SequenceTracker tracker;
    tracker.observe(0xFFFFFFFEu);
    tracker.observe(0xFFFFFFFFu);
    tracker.observe(0);
    tracker.observe(1);
    EXPECT_EQ(tracker.droppedPackets(), 0u);
}

TEST(SequenceTracker, TreatsBackwardJumpAsRestart) {
    SequenceTracker tracker;
    tracker.observe(500);
    tracker.observe(0);
    tracker.observe(1);
    EXPECT_EQ(tracker.droppedPackets(), 0u);
}