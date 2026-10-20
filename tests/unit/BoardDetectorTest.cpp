// VIEW-001: detect the board by USB VID/PID.
#include "device/BoardDetector.h"
#include "device/UsbHardwareId.h"
#include "fakes/FakePortEnumerator.h"

#include <gtest/gtest.h>

using namespace coral;
using coral::testing::FakePortEnumerator;
using coral::testing::boardPort;

namespace {
constexpr std::uint16_t kOtherVendorId = 0x0403;
constexpr std::uint16_t kOtherProductId = 0x6001;
}  // namespace

TEST(BoardDetector, SelectsPortMatchingVidPid) {
    FakePortEnumerator enumerator;
    enumerator.setPorts({SerialPortInfo{"COM3", kOtherVendorId, kOtherProductId}, boardPort("COM17")});
    BoardDetector detector(enumerator);

    const auto result = detector.findBoard();

    ASSERT_TRUE(result.ok()) << result.error();
    ASSERT_TRUE(result.value().has_value());
    EXPECT_EQ(result.value()->portName, "COM17");
}

TEST(BoardDetector, ReportsNotFoundWhenNoMatch) {
    FakePortEnumerator enumerator;
    enumerator.setPorts({SerialPortInfo{"COM3", kOtherVendorId, kOtherProductId}});
    BoardDetector detector(enumerator);

    const auto result = detector.findBoard();

    ASSERT_TRUE(result.ok()) << result.error();
    EXPECT_FALSE(result.value().has_value());
}

TEST(UsbHardwareId, ParsesVidPidFromWindowsHardwareId) {
    const auto id = parseUsbHardwareId("USB\\VID_18D1&PID_9308&REV_0100&MI_00");

    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(id->vendorId, 0x18D1);
    EXPECT_EQ(id->productId, 0x9308);
}

TEST(UsbHardwareId, RejectsNonUsbHardwareId) {
    EXPECT_FALSE(parseUsbHardwareId("ACPI\\PNP0501").has_value());
}