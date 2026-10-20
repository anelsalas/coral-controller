// D-10: CRC-32 (IEEE) matches Boost.CRC so PC and firmware agree.
#include "protocol/Crc32.h"

#include <boost/crc.hpp>
#include <gtest/gtest.h>

#include <string>

using coral::protocol::crc32;

TEST(Crc32, MatchesStandardCheckValue) {
    const std::string check = "123456789";
    EXPECT_EQ(crc32(reinterpret_cast<const std::uint8_t*>(check.data()), check.size()), 0xCBF43926u);
}

TEST(Crc32, MatchesBoostCrc32AndIsIncremental) {
    std::vector<std::uint8_t> data(1000);
    for (std::size_t i = 0; i < data.size(); ++i) data[i] = static_cast<std::uint8_t>(i * 7 + 3);
    boost::crc_32_type boostCrc;
    boostCrc.process_bytes(data.data(), data.size());

    const std::uint32_t whole = crc32(data.data(), data.size());
    const std::uint32_t split = crc32(data.data() + 400, 600, crc32(data.data(), 400));

    EXPECT_EQ(whole, boostCrc.checksum());
    EXPECT_EQ(split, whole);
}