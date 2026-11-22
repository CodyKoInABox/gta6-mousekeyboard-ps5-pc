#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>

#include "core/account_id.h"
#include "core/pad_state.h"
#include "core/pairing.h"
#include "core/remote_play.h"

struct chiaki_controller_state_t;
struct chiaki_event_t;
struct chiaki_session_t;

namespace pskb::chiaki_backend {

// Maps our frame onto the protocol's controller state. Bits are identical by construction.
void ToControllerState(const PadState& pad, chiaki_controller_state_t& out);

// Maps a protocol quit reason to what the link layer should do next.
LinkError ClassifyQuit(int chiakiQuitReason);

// Plain-language message for a protocol quit reason. Never includes keys or identifiers.
std::string QuitMessage(int chiakiQuitReason);

struct RegisterResult {
    bool ok = false;
    std::string message;
    PairingInfo pairing;  // valid only when ok
};

// One-time pairing. The PS5 must be awake and showing its 8-digit PIN (Settings > System >
// Remote Play > Pair Device). The PIN expires after a few minutes.
RegisterResult RegisterConsole(const std::string& host, const AccountId& account, const std::string& pin);

}  // namespace pskb::chiaki_backend

namespace pskb {

// Input-only Remote Play: no video or audio is requested or decoded. The PS5 still streams
// A/V packets, which the protocol layer discards.
class ChiakiBackend final : public IRemotePlayBackend {
public:
    ChiakiBackend();
    ~ChiakiBackend() override;

    ChiakiBackend(const ChiakiBackend&) = delete;
    ChiakiBackend& operator=(const ChiakiBackend&) = delete;

    ConnectResult Connect(const PairingInfo& pairing) override;
    void SendPad(const PadState& pad) override;
    bool IsConnected() const override;
    void Disconnect() override;
    void RequestCancel() override;

    void SetConnectTimeoutSeconds(unsigned int seconds) { connectTimeoutSec_ = seconds; }

private:
    struct EventState {
        bool connected = false;
        bool quit = false;
        bool loginPin = false;
        int quitReason = 0;
        std::string quitRemote;
    };

    static void OnEvent(chiaki_event_t* event, void* user);
    static void TearDown(chiaki_session_t* session);
    void ResetEvents();
    ConnectResult WaitForStart(chiaki_session_t* session, const std::string& name);
    chiaki_session_t* TakeSession();

    mutable std::mutex mutex_;          // guards session_; never held while waiting
    chiaki_session_t* session_ = nullptr;
    std::atomic<bool> connected_{false};
    std::atomic<bool> cancelRequested_{false};

    // Event flags only. chiaki may invoke OnEvent while holding its own locks, so nothing
    // here may call back into chiaki or take mutex_.
    std::mutex eventMutex_;
    std::condition_variable eventCv_;
    EventState events_;

    unsigned int connectTimeoutSec_ = 40;
};

}  // namespace pskb
