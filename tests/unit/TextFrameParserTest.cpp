// VIEW-003, VIEW-004: text protocol "FRAME:<base64 JPEG>\r\n".
#include "protocol/TextFrameParser.h"
#include "fakes/RecordingListeners.h"
#include "fakes/TestImages.h"

#include <gtest/gtest.h>

using namespace coral;
using coral::testing::RecordingTextListener;

namespace {
void feedText(TextFrameParser& parser, const std::string& text) {
    parser.feed(std::span(reinterpret_cast<const std::uint8_t*>(text.data()), text.size()));
}
const std::vector<std::uint8_t> kPayload{0xFF, 0xD8, 0x01, 0x02, 0x03, 0xFF, 0xD9};
}  // namespace

TEST(TextFrameParser, DecodesValidLine) {
    RecordingTextListener listener;
    TextFrameParser parser(listener);

    feedText(parser, coral::testing::frameLine(kPayload));

    ASSERT_EQ(listener.frames.size(), 1u);
    EXPECT_EQ(listener.frames[0], kPayload);
    EXPECT_TRUE(listener.errors.empty());
    EXPECT_EQ(parser.counters().frames, 1u);
}

TEST(TextFrameParser, HandlesLineSplitAcrossReads) {
    RecordingTextListener listener;
    TextFrameParser parser(listener);
    const std::string line = coral::testing::frameLine(kPayload);

    feedText(parser, line.substr(0, 3));
    feedText(parser, line.substr(3, 5));
    EXPECT_TRUE(listener.frames.empty());
    feedText(parser, line.substr(8));

    ASSERT_EQ(listener.frames.size(), 1u);
    EXPECT_EQ(listener.frames[0], kPayload);
}

TEST(TextFrameParser, RoutesNonFrameLinesToLog) {
    RecordingTextListener listener;
    TextFrameParser parser(listener);

    feedText(parser, "Camera serial streaming!\r\n");

    ASSERT_EQ(listener.logLines.size(), 1u);
    EXPECT_EQ(listener.logLines[0], "Camera serial streaming!");
    EXPECT_TRUE(listener.frames.empty());
    EXPECT_EQ(parser.counters().logLines, 1u);
}

TEST(TextFrameParser, RejectsInvalidBase64) {
    RecordingTextListener listener;
    TextFrameParser parser(listener);

    feedText(parser, "FRAME:not*base64!\r\n");
    feedText(parser, coral::testing::frameLine(kPayload));

    ASSERT_EQ(listener.errors.size(), 1u);
    EXPECT_EQ(listener.errors[0], ParseErrorKind::InvalidBase64);
    EXPECT_EQ(parser.counters().invalidBase64, 1u);
    EXPECT_EQ(listener.frames.size(), 1u) << "stream continues after a bad frame";
}

TEST(TextFrameParser, RejectsOverlongLine) {
    constexpr std::size_t kSmallMaxLineLength = 16;
    RecordingTextListener listener;
    TextFrameParser parser(listener, kSmallMaxLineLength);

    feedText(parser, std::string(40, 'A'));
    feedText(parser, "AAAA\r\n");
    feedText(parser, "FRAME:AQID\r\n");

    ASSERT_EQ(listener.errors.size(), 1u);
    EXPECT_EQ(listener.errors[0], ParseErrorKind::OverlongLine);
    EXPECT_EQ(parser.counters().overlongLines, 1u);
    ASSERT_EQ(listener.frames.size(), 1u);
    EXPECT_EQ(listener.frames[0], (std::vector<std::uint8_t>{1, 2, 3}));
}