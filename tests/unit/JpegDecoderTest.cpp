// VIEW-004, VIEW-005: JPEG decoding.
#include "imaging/JpegDecoder.h"
#include "fakes/TestImages.h"

#include <gtest/gtest.h>

using namespace coral;

TEST(JpegDecoder, DecodesFixtureTo324x324) {
    const auto jpeg = coral::testing::boardSizedJpeg();
    JpegDecoder decoder;

    const auto result = decoder.decode(jpeg);

    ASSERT_TRUE(result.ok()) << result.error();
    EXPECT_EQ(result.value().cols, coral::testing::kBoardImageSize);
    EXPECT_EQ(result.value().rows, coral::testing::kBoardImageSize);
    EXPECT_EQ(result.value().channels(), 3);
}

TEST(JpegDecoder, ReportsCorruptImage) {
    const std::vector<std::uint8_t> garbage{0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    JpegDecoder decoder;

    const auto result = decoder.decode(garbage);

    EXPECT_FALSE(result.ok());
    EXPECT_FALSE(result.error().empty());
}

TEST(JpegDecoder, ReportsEmptyInput) {
    JpegDecoder decoder;
    EXPECT_FALSE(decoder.decode({}).ok());
}