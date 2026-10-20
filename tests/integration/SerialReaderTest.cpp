// VIEW-002: background reading never blocks the caller.
#include "device/SerialReader.h"
#include "fakes/FakeSerialPort.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

using namespace coral;
using namespace std::chrono_literals;

namespace {
template <typename Predicate>
bool waitUntil(Predicate predicate, std::chrono::milliseconds limit = 3s) {
    const auto deadline = std::chrono::steady_clock::now() + limit;
    while (!predicate()) {
        if (std::chrono::steady_clock::now() > deadline) return false;
        std::this_thread::sleep_for(1ms);
    }
    return true;
}
}  // namespace

TEST(SerialReader, DeliversDataWithoutBlockingCaller) {
    constexpr auto kSlowRead = 200ms;
    coral::testing::FakeSerialPort port;
    port.setReadDelay(kSlowRead);
    port.queueData("hello");
    std::mutex mutex;
    std::string received;
    SerialReader reader(
        port,
        [&](std::span<const std::uint8_t> bytes) {
            std::lock_guard lock(mutex);
            received.append(bytes.begin(), bytes.end());
        },
        [](const std::string&) {});

    const auto before = std::chrono::steady_clock::now();
    reader.start();
    const auto startDuration = std::chrono::steady_clock::now() - before;

    EXPECT_LT(startDuration, kSlowRead) << "start() must return before the slow read completes";
    EXPECT_TRUE(waitUntil([&] {
        std::lock_guard lock(mutex);
        return received == "hello";
    }));
    reader.stop();
}

TEST(SerialReader, ReportsReadFailureAndStops) {
    coral::testing::FakeSerialPort port;
    port.queueFailure("device unplugged");
    std::atomic<bool> failed{false};
    SerialReader reader(port, [](std::span<const std::uint8_t>) {}, [&](const std::string& message) {
        EXPECT_EQ(message, "device unplugged");
        failed = true;
    });

    reader.start();

    EXPECT_TRUE(waitUntil([&] { return failed.load(); }));
    EXPECT_TRUE(waitUntil([&] { return !reader.isRunning(); }));
}