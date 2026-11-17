#include "core/signal/signal.hpp"
#include <gtest/gtest.h>

using namespace northern::core;

TEST(SignalTest, LambdaConnectionAndEmission) {
    Signal<int, int> signal;
    int sum = 0;

    auto conn = signal.connect([&sum](int a, int b) { sum = a + b; });
    EXPECT_TRUE(conn.isConnected());

    signal.emit(10, 20);
    EXPECT_EQ(sum, 30);

    conn.disconnect();
    EXPECT_FALSE(conn.isConnected());

    signal.emit(5, 5);
    EXPECT_EQ(sum, 30);
}

class TestReceiver : public Trackable {
public:
    int lastValue = 0;
    void onValue(int val) { lastValue = val; }
};

TEST(SignalTest, MemberFunctionAndTrackableAutoDisconnect) {
    Signal<int> signal;

    {
        TestReceiver receiver;
        signal.connect(&receiver, &TestReceiver::onValue);

        signal.emit(42);
        EXPECT_EQ(receiver.lastValue, 42);
    } // receiver destroyed here

    // Emitting after receiver destruction must be safe and not crash
    signal.emit(99);
}

TEST(SignalTest, ScopedConnectionDisconnectsOnDestruction) {
    Signal<int> signal;
    int received = 0;

    {
        ScopedConnection scoped(signal.connect([&received](int val) { received = val; }));
        EXPECT_TRUE(scoped.isConnected());
        signal.emit(15);
        EXPECT_EQ(received, 15);
    } // scoped destroyed here

    signal.emit(30);
    EXPECT_EQ(received, 15);
}

TEST(SignalTest, SafeDisconnectionDuringEmission) {
    Signal<int> signal;
    Connection conn;
    int invocations = 0;

    conn = signal.connect([&](int) {
        invocations++;
        conn.disconnect();
    });

    signal.emit(1);
    signal.emit(2);
    EXPECT_EQ(invocations, 1);
}
