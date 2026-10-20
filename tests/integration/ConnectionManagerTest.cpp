// VIEW-012: reconnect after the port is lost, without restarting.
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
class ConnectionManagerTest : public ::testing::Test {
protected:
    template <typename Predicate>
    bool tickUntil(Predicate predicate, std::chrono::milliseconds limit = 3s) {
        const auto deadline = std::chrono::steady_clock::now() + limit;
        while (!predicate()) {
            if (std::chrono::steady_clock::now() > deadline) return false;
            manager.tick();
            std::this_thread::sleep_for(1ms);
        }
        return true;
    }

    std::string received() {
        std::lock_guard lock(mutex);
        return bytes;
    }

    FakePortEnumerator enumerator;
    FakeSerialPort port;
    FakeClock clock;
    RecordingConnectionListener listener;
    std::mutex mutex;
    std::string bytes;
    ConnectionSettings settings{};
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

    manager.tick();

    EXPECT_EQ(manager.state(), ConnectionState::Connected);
    EXPECT_EQ(port.openedPortName(), "COM8");
    EXPECT_EQ(port.openedBaudRate(), kDefaultBaudRate);
    manager.shutdown();
}

TEST_F(ConnectionManagerTest, StaysSearchingWhenNoBoard) {
    manager.tick();
    EXPECT_EQ(manager.state(), ConnectionState::Searching);
    EXPECT_EQ(port.openCount(), 0);
}

TEST_F(ConnectionManagerTest, ReconnectsAfterPortLoss) {
    enumerator.setPorts({boardPort("COM8")});
    port.queueData("abc");
    port.queueFailure("device unplugged");
    port.queueData("def");

    manager.tick();
    ASSERT_EQ(manager.state(), ConnectionState::Connected);
    ASSERT_TRUE(tickUntil([&] { return manager.state() == ConnectionState::Searching; }));
    EXPECT_EQ(listener.disconnectReasons(), std::vector<std::string>{"device unplugged"});

    manager.tick();
    EXPECT_EQ(manager.state(), ConnectionState::Searching) << "waits for the retry interval";
    clock.advance(settings.retryInterval);
    manager.tick();

    EXPECT_EQ(manager.state(), ConnectionState::Connected);
    EXPECT_EQ(listener.connectedCount(), 2u);
    EXPECT_TRUE(tickUntil([&] { return received() == "abcdef"; }));
    manager.shutdown();
}