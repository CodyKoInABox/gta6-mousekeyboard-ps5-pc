#include <atomic>
#include <chrono>
#include <deque>
#include <initializer_list>
#include <mutex>
#include <thread>
#include <vector>

#include "core/link_manager.h"
#include "test_framework.h"

using namespace pskb;

namespace {

// Scripted stand-in for the PS5 transport. Records every call so tests can assert on the
// exact sequence: duplicate connects, pad frames, and release on disconnect.
class MockBackend : public IRemotePlayBackend {
public:
    ConnectResult Connect(const PairingInfo&) override {
        {
            std::lock_guard lock(m_);
            ++connectCalls;
            if (!script_.empty()) {
                const auto step = script_.front();
                script_.pop_front();
                if (step.ok) {
                    connected_ = true;
                    ++successes;
                    return {true, LinkError::None, "Conectado (teste)."};
                }
                connected_ = false;
                return {false, step.error, "falha de teste"};
            }
        }
        // Default: stay in a "connecting" state until cancelled.
        for (int i = 0; i < 2000; ++i) {
            {
                std::lock_guard lock(m_);
                if (cancelled_) break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        {
            std::lock_guard lock(m_);
            cancelled_ = false;
        }
        return {false, LinkError::Cancelled, "cancelado"};
    }

    void SendPad(const PadState& pad) override {
        std::lock_guard lock(m_);
        if (!connected_) return;
        ++padsSent;
        lastPad = pad;
    }

    bool IsConnected() const override {
        std::lock_guard lock(m_);
        return connected_;
    }

    void Disconnect() override {
        std::lock_guard lock(m_);
        ++disconnectCalls;
        connected_ = false;
    }

    void RequestCancel() override {
        std::lock_guard lock(m_);
        cancelled_ = true;
    }

    // Test controls.
    struct Step {
        bool ok;
        LinkError error;
    };
    void Script(std::initializer_list<Step> steps) {
        std::lock_guard lock(m_);
        script_.assign(steps.begin(), steps.end());
    }
    void DropConnection() {
        std::lock_guard lock(m_);
        connected_ = false;
    }

    std::atomic<int> connectCalls{0};
    std::atomic<int> disconnectCalls{0};
    std::atomic<int> successes{0};
    std::atomic<int> padsSent{0};
    PadState lastPad{};

private:
    mutable std::mutex m_;
    std::deque<Step> script_;
    bool connected_ = false;
    bool cancelled_ = false;
};

constexpr auto kTimeout = std::chrono::seconds(5);

template <typename Pred>
bool WaitFor(Pred pred) {
    const auto deadline = std::chrono::steady_clock::now() + kTimeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (pred()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return pred();
}

PairingInfo ValidPairing() {
    PairingInfo p;
    p.target = 1100;
    p.registKey[0] = 'a';
    p.morning[0] = 1;
    p.host = "192.168.15.40";
    return p;
}

RetryPolicy FastPolicy() {
    RetryPolicy r;
    r.baseDelayMs = 5;
    r.maxDelayMs = 20;
    r.refusedDelayMs = 5;
    return r;
}

LinkManager::PairingLoader Loader() {
    return [] { return std::optional<PairingInfo>(ValidPairing()); };
}

}  // namespace

PSKB_TEST(ConnectReachesConnectedAndForwardsPads) {
    MockBackend backend;
    backend.Script({{true, LinkError::None}});
    LinkManager link(backend, Loader(), FastPolicy());

    link.SubmitPad(PadState{});  // before connect: dropped
    CHECK_EQ(backend.padsSent.load(), 0);

    link.Connect();
    CHECK(WaitFor([&] { return link.Status().state == LinkState::Connected; }));

    PadState frame;
    frame.buttons = PadCross;
    link.SubmitPad(frame);
    CHECK_EQ(backend.padsSent.load(), 1);
    CHECK(backend.lastPad == frame);
    link.Disconnect();
}

PSKB_TEST(RepeatedConnectCallsNeverOpenSecondSession) {
    MockBackend backend;
    backend.Script({{true, LinkError::None}});
    LinkManager link(backend, Loader(), FastPolicy());

    link.Connect();
    link.Connect();
    link.Connect();
    CHECK(WaitFor([&] { return link.Status().state == LinkState::Connected; }));
    link.Connect();
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    CHECK_EQ(backend.connectCalls.load(), 1);
    link.Disconnect();
}

PSKB_TEST(DisconnectReleasesPadAndStopsForwarding) {
    MockBackend backend;
    backend.Script({{true, LinkError::None}});
    LinkManager link(backend, Loader(), FastPolicy());
    link.Connect();
    CHECK(WaitFor([&] { return link.Status().state == LinkState::Connected; }));

    PadState held;
    held.r2 = 255;
    link.SubmitPad(held);
    CHECK_EQ(backend.padsSent.load(), 1);

    link.Disconnect();
    CHECK(WaitFor([&] { return link.Status().state == LinkState::Idle; }));
    CHECK(WaitFor([&] { return backend.disconnectCalls.load() >= 1; }));
    CHECK(backend.lastPad.IsNeutral());

    const int sentAfterRelease = backend.padsSent.load();
    link.SubmitPad(held);
    CHECK_EQ(backend.padsSent.load(), sentAfterRelease);
}

PSKB_TEST(DroppedSessionReconnectsAutomatically) {
    MockBackend backend;
    backend.Script({{true, LinkError::None}, {true, LinkError::None}});
    LinkManager link(backend, Loader(), FastPolicy());
    link.Connect();
    CHECK(WaitFor([&] { return link.Status().state == LinkState::Connected; }));

    backend.DropConnection();
    CHECK(WaitFor([&] { return backend.connectCalls.load() == 2 && link.Status().state == LinkState::Connected; }));
    CHECK_EQ(backend.successes.load(), 2);
    link.Disconnect();
}

PSKB_TEST(DroppedSessionWithoutAutoReconnectFailsCleanly) {
    MockBackend backend;
    backend.Script({{true, LinkError::None}});
    RetryPolicy policy = FastPolicy();
    policy.autoReconnect = false;
    LinkManager link(backend, Loader(), policy);
    link.Connect();
    CHECK(WaitFor([&] { return link.Status().state == LinkState::Connected; }));

    backend.DropConnection();
    CHECK(WaitFor([&] { return link.Status().state == LinkState::Failed; }));
    CHECK_EQ(backend.connectCalls.load(), 1);
}

PSKB_TEST(TransientFailuresRetryUntilConnected) {
    MockBackend backend;
    backend.Script({{false, LinkError::NotFound}, {false, LinkError::Asleep}, {true, LinkError::None}});
    LinkManager link(backend, Loader(), FastPolicy());
    link.Connect();
    CHECK(WaitFor([&] { return link.Status().state == LinkState::Connected; }));
    CHECK_EQ(backend.connectCalls.load(), 3);
}

PSKB_TEST(RefusedSessionRetriesAfterWait) {
    MockBackend backend;
    backend.Script({{false, LinkError::Refused}, {true, LinkError::None}});
    LinkManager link(backend, Loader(), FastPolicy());
    link.Connect();
    CHECK(WaitFor([&] { return link.Status().state == LinkState::Connected; }));
    CHECK_EQ(backend.connectCalls.load(), 2);
}

PSKB_TEST(LoginPasscodeIsFatalAndDoesNotRetry) {
    MockBackend backend;
    backend.Script({{false, LinkError::LoginPasscode}, {true, LinkError::None}});
    LinkManager link(backend, Loader(), FastPolicy());
    link.Connect();
    CHECK(WaitFor([&] { return link.Status().state == LinkState::Failed; }));
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
    CHECK_EQ(backend.connectCalls.load(), 1);
    CHECK(link.Status().error == LinkError::LoginPasscode);
}

PSKB_TEST(MissingPairingFailsWithoutConnecting) {
    MockBackend backend;
    LinkManager link(backend, [] { return std::optional<PairingInfo>(); }, FastPolicy());
    link.Connect();
    CHECK(WaitFor([&] { return link.Status().state == LinkState::Failed; }));
    CHECK(link.Status().error == LinkError::NotPaired);
    CHECK_EQ(backend.connectCalls.load(), 0);
}

PSKB_TEST(CancelDuringConnectReturnsToIdle) {
    MockBackend backend;  // no script: Connect blocks until cancelled
    LinkManager link(backend, Loader(), FastPolicy());
    link.Connect();
    CHECK(WaitFor([&] { return backend.connectCalls.load() == 1; }));
    link.Disconnect();
    CHECK(WaitFor([&] { return link.Status().state == LinkState::Idle; }));
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    CHECK_EQ(backend.connectCalls.load(), 1);
}

PSKB_TEST(StatusListenerSeesConnectingThenConnected) {
    MockBackend backend;
    backend.Script({{true, LinkError::None}});
    LinkManager link(backend, Loader(), FastPolicy());
    std::mutex m;
    std::vector<LinkState> seen;
    link.SetStatusListener([&](const LinkStatus& s) {
        std::lock_guard lock(m);
        seen.push_back(s.state);
    });
    link.Connect();
    CHECK(WaitFor([&] { return link.Status().state == LinkState::Connected; }));
    std::lock_guard lock(m);
    bool sawConnecting = false;
    bool sawConnected = false;
    for (auto s : seen) {
        sawConnecting = sawConnecting || s == LinkState::Connecting;
        sawConnected = sawConnected || s == LinkState::Connected;
    }
    CHECK(sawConnecting);
    CHECK(sawConnected);
}

PSKB_TEST(DestroyingWhileConnectedReleasesPad) {
    MockBackend backend;
    backend.Script({{true, LinkError::None}});
    {
        LinkManager link(backend, Loader(), FastPolicy());
        link.Connect();
        CHECK(WaitFor([&] { return link.Status().state == LinkState::Connected; }));
    }
    CHECK(backend.disconnectCalls.load() >= 1);
}
