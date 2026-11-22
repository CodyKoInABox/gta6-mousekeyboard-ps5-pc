#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include "core/pad_state.h"
#include "core/pairing.h"
#include "core/remote_play.h"

namespace pskb {

enum class LinkState : unsigned char {
    Idle,        // not wanted: user has not pressed Jogar, or pressed Parar
    Connecting,
    Connected,
    Retrying,    // waiting before the next attempt
    Failed,      // fatal error, shown to the user until they press Jogar again
};

const char* LinkStateName(LinkState state) noexcept;

struct LinkStatus {
    LinkState state = LinkState::Idle;
    LinkError error = LinkError::None;
    std::string message;
    unsigned int attempt = 0;
};

struct RetryPolicy {
    unsigned int baseDelayMs = 2000;
    unsigned int maxDelayMs = 30000;
    unsigned int refusedDelayMs = 10000;
    unsigned int networkCheckMs = 3000;  // how often to re-check the network while it is down
    bool autoReconnect = true;
};

// True when the PC has a usable network connection. Injected so tests do not depend on the
// real adapters.
using NetworkProbe = std::function<bool()>;

// Owns the connection lifecycle: one worker thread, at most one Connect() in flight, and no
// second session while one is connected. Pad frames are forwarded only while Connected.
class LinkManager {
public:
    using PairingLoader = std::function<std::optional<PairingInfo>()>;
    using StatusListener = std::function<void(const LinkStatus&)>;

    // A null probe means "always connected", which is what the unit tests rely on.
    LinkManager(IRemotePlayBackend& backend, PairingLoader loader, RetryPolicy policy = {},
                NetworkProbe networkProbe = {});
    ~LinkManager();

    LinkManager(const LinkManager&) = delete;
    LinkManager& operator=(const LinkManager&) = delete;

    void SetStatusListener(StatusListener listener);

    // Jogar. No-op while already connecting or connected.
    void Connect();

    // Parar. Releases every button and stops the session. Returns immediately.
    void Disconnect();

    // Called from the input thread at the engine's rate. Dropped unless Connected.
    void SubmitPad(const PadState& pad);

    LinkStatus Status() const;
    bool IsConnected() const;

private:
    void Run();
    void SetState(LinkState state, LinkError error, std::string message, unsigned int attempt);
    bool Wanted() const;
    bool NetworkUp() const;
    void WaitRetry(unsigned int delayMs);
    void HoldWhileConnected();
    void Notify(const LinkStatus& status);

    IRemotePlayBackend& backend_;
    PairingLoader loader_;
    RetryPolicy policy_;
    NetworkProbe networkProbe_;

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    StatusListener listener_;
    LinkStatus status_;
    bool wantConnected_ = false;
    bool stop_ = false;
    bool padEnabled_ = false;

    std::thread worker_;
};

}  // namespace pskb
