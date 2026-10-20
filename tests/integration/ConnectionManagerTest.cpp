// VIEW-002 / VIEW-012: the connection runs on its own worker; the UI thread never blocks on the device.
#include "device/ConnectionManager.h"
#include "fakes/FakeClock.h"
#include "fakes/FakePortEnumerator.h"
#include "fakes/FakeSerialPort.h"
#include "fakes/RecordingListeners.h"

#include <gtest/gtest.h>

#include <mutex>
#include <thread>

using namespace coral;
using namespace coral::testing;
using namespace std::chrono_literals;

namespace {
constexpr auto kUiBudget = 100ms;  // far below what a user perceives as "not responding"

template <typename Predicate>
bool waitUntil(Predicate predicate, std::chrono::milliseconds limit = 3s) {
    const auto deadline = std::chrono::steady_clock::now() + limit;
    while (!predicate()) {
        if (std::chrono::steady_clock::now() > deadline) return false;
        std::this_thread::sleep_for(1ms);
    }
    return true;
}

template <typename Action>
std::chrono::milliseconds timed(Action action) {
    const auto start = std::chrono::steady_clock::now();
    action();
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
}

class ConnectionManagerTest : public ::testing::Test {
protected:
    void TearDown() override {
        port.releaseBlockedCalls();
        manager.shutdown();
    }

    std::string received() {
        std::lock_guard lock(mutex);
        return bytes;
    }
    bool isConnected() { return manager.state() == ConnectionState::Connected; }

    FakePortEnumerator enumerator;
    FakeSerialPort port;
    FakeClock clock;
    RecordingConnectionListener listener;
    std::mutex mutex;
    std::string bytes;
    ConnectionSettings settings{kDefaultBaudRate, 1000ms, 200ms};
    ConnectionManager manager{enumerator, port, clock,
                              [this](std::span<const std::uint8_t> data) {
                                  std::lock_guard lock(mutex);
                                  bytes.append(data.begin(), data.end());
                              },
                              listener, settings};
};
}  // namespace

TEST_F(ConnectionManagerTest, ConnectsToDetectedBoardAtConfiguredBaudRate) {
    enumerator.setPorts({boardPort("COM8")});

    manager.start();

    ASSERT_TRUE(waitUntil([&] { return isConnected(); }));
    EXPECT_EQ(port.openedPortName(), "COM8");
    EXPECT_EQ(port.openedBaudRate(), kDefaultBaudRate);
}

TEST_F(ConnectionManagerTest, StaysSearchingWhenNoBoard) {
    manager.start();
    std::this_thread::sleep_for(50ms);

    EXPECT_EQ(manager.state(), ConnectionState::Searching);
    EXPECT_EQ(port.openCount(), 0);
}

TEST_F(ConnectionManagerTest, DeliversDataAfterConnecting) {
    enumerator.setPorts({boardPort("COM8")});
    port.queueData("hello");

    manager.start();

    EXPECT_TRUE(waitUntil([&] { return received() == "hello"; }));
    EXPECT_EQ(listener.connectedCount(), 1u);
}

TEST_F(ConnectionManagerTest, ReconnectsAfterPortLossOnceRetryIntervalPassed) {
    enumerator.setPorts({boardPort("COM8")});
    port.queueData("abc");
    port.queueFailure("device unplugged");
    port.queueData("def");

    manager.start();
    ASSERT_TRUE(waitUntil([&] { return listener.disconnectReasons().size() == 1; }));
    EXPECT_EQ(listener.disconnectReasons(), std::vector<std::string>{"device unplugged"});
    std::this_thread::sleep_for(50ms);
    EXPECT_EQ(port.openCount(), 1) << "waits for the retry interval";

    clock.advance(settings.retryInterval);

    EXPECT_TRUE(waitUntil([&] { return received() == "abcdef"; }));
    EXPECT_EQ(listener.connectedCount(), 2u);
}

TEST_F(ConnectionManagerTest, StartReturnsImmediatelyWhileOpenIsStuck) {
    enumerator.setPorts({boardPort("COM8")});
    port.blockOpen();

    EXPECT_LT(timed([&] { manager.start(); }), kUiBudget);
    std::this_thread::sleep_for(20ms);
    EXPECT_LT(timed([&] { (void)manager.state(); }), kUiBudget);
}

TEST_F(ConnectionManagerTest, ShutdownGivesUpOnStuckOpenWithinTimeout) {
    enumerator.setPorts({boardPort("COM8")});
    port.blockOpen();
    manager.start();
    std::this_thread::sleep_for(20ms);

    bool stoppedCleanly = true;
    const auto took = timed([&] { stoppedCleanly = manager.shutdown(); });

    EXPECT_FALSE(stoppedCleanly);
    EXPECT_LT(took, settings.shutdownTimeout + kUiBudget);
    port.releaseBlockedCalls();
    EXPECT_TRUE(waitUntil([&] { return port.returnedCalls() == 1; }));
    std::this_thread::sleep_for(20ms);
    EXPECT_EQ(listener.connectedCount(), 0u) << "an abandoned worker must not call listeners";
}

TEST_F(ConnectionManagerTest, ShutdownGivesUpOnStuckCloseWithinTimeout) {
    enumerator.setPorts({boardPort("COM8")});
    manager.start();
    ASSERT_TRUE(waitUntil([&] { return isConnected(); }));
    port.blockClose();

    bool stoppedCleanly = true;
    const auto took = timed([&] { stoppedCleanly = manager.shutdown(); });

    EXPECT_FALSE(stoppedCleanly);
    EXPECT_LT(took, settings.shutdownTimeout + kUiBudget);
}

TEST_F(ConnectionManagerTest, ShutdownStopsCleanlyWhenDeviceResponds) {
    enumerator.setPorts({boardPort("COM8")});
    manager.start();
    ASSERT_TRUE(waitUntil([&] { return isConnected(); }));

    EXPECT_TRUE(manager.shutdown());
    EXPECT_FALSE(port.isOpen());
    EXPECT_EQ(listener.disconnectReasons(), std::vector<std::string>{"application shutdown"});
}