#include "core/link_manager.h"

#include <algorithm>
#include <chrono>

namespace pskb {
namespace {

using namespace std::chrono_literals;

PadState NeutralPad() { return PadState{}; }

}  // namespace

const char* LinkStateName(LinkState state) noexcept {
    switch (state) {
        case LinkState::Idle: return "idle";
        case LinkState::Connecting: return "connecting";
        case LinkState::Connected: return "connected";
        case LinkState::Retrying: return "retrying";
        case LinkState::Failed: return "failed";
    }
    return "unknown";
}

bool IsFatal(LinkError error) noexcept {
    return error == LinkError::NotPaired || error == LinkError::LoginPasscode;
}

const char* LinkErrorName(LinkError error) noexcept {
    switch (error) {
        case LinkError::None: return "none";
        case LinkError::NotPaired: return "not_paired";
        case LinkError::NotFound: return "not_found";
        case LinkError::Asleep: return "asleep";
        case LinkError::Refused: return "refused";
        case LinkError::LoginPasscode: return "login_passcode";
        case LinkError::NetworkUnavailable: return "network_unavailable";
        case LinkError::Protocol: return "protocol";
        case LinkError::Cancelled: return "cancelled";
        case LinkError::Other: return "other";
    }
    return "unknown";
}

LinkManager::LinkManager(IRemotePlayBackend& backend, PairingLoader loader, RetryPolicy policy)
    : backend_(backend), loader_(std::move(loader)), policy_(policy) {
    worker_ = std::thread([this] { Run(); });
}

LinkManager::~LinkManager() {
    {
        std::lock_guard lock(mutex_);
        stop_ = true;
        wantConnected_ = false;
        padEnabled_ = false;
    }
    backend_.RequestCancel();
    cv_.notify_all();
    if (worker_.joinable()) worker_.join();
    backend_.Disconnect();
}

void LinkManager::SetStatusListener(StatusListener listener) {
    std::lock_guard lock(mutex_);
    listener_ = std::move(listener);
}

void LinkManager::Connect() {
    {
        std::lock_guard lock(mutex_);
        if (wantConnected_ || stop_) return;
        wantConnected_ = true;
        status_ = {LinkState::Connecting, LinkError::None, "Conectando\u2026", 0};
    }
    cv_.notify_all();
    Notify(Status());
}

void LinkManager::Disconnect() {
    {
        std::lock_guard lock(mutex_);
        wantConnected_ = false;
        padEnabled_ = false;
        status_ = {LinkState::Idle, LinkError::Cancelled, "Desconectado.", 0};
    }
    backend_.SendPad(NeutralPad());
    backend_.RequestCancel();
    cv_.notify_all();
    Notify(Status());
}

void LinkManager::SubmitPad(const PadState& pad) {
    bool enabled;
    {
        std::lock_guard lock(mutex_);
        enabled = padEnabled_;
    }
    if (enabled) backend_.SendPad(pad);
}

LinkStatus LinkManager::Status() const {
    std::lock_guard lock(mutex_);
    return status_;
}

bool LinkManager::IsConnected() const {
    std::lock_guard lock(mutex_);
    return padEnabled_;
}

void LinkManager::SetState(LinkState state, LinkError error, std::string message, unsigned int attempt) {
    LinkStatus snapshot;
    {
        std::lock_guard lock(mutex_);
        status_ = {state, error, std::move(message), attempt};
        snapshot = status_;
    }
    Notify(snapshot);
}

void LinkManager::Notify(const LinkStatus& status) {
    StatusListener listener;
    {
        std::lock_guard lock(mutex_);
        listener = listener_;
    }
    if (listener) listener(status);
}

bool LinkManager::Wanted() const {
    std::lock_guard lock(mutex_);
    return wantConnected_ && !stop_;
}

void LinkManager::WaitRetry(unsigned int delayMs) {
    std::unique_lock lock(mutex_);
    cv_.wait_for(lock, std::chrono::milliseconds(delayMs), [this] { return stop_ || !wantConnected_; });
}

void LinkManager::HoldWhileConnected() {
    for (;;) {
        {
            std::unique_lock lock(mutex_);
            cv_.wait_for(lock, 100ms, [this] { return stop_ || !wantConnected_; });
            if (stop_ || !wantConnected_) return;
        }
        if (!backend_.IsConnected()) return;
    }
}

void LinkManager::Run() {
    for (;;) {
        {
            std::unique_lock lock(mutex_);
            cv_.wait(lock, [this] { return stop_ || wantConnected_; });
            if (stop_) return;
        }

        unsigned int attempt = 0;
        bool keepGoing = true;
        while (keepGoing) {
            const auto pairing = loader_ ? loader_() : std::nullopt;
            if (!pairing) {
                {
                    std::lock_guard lock(mutex_);
                    wantConnected_ = false;
                }
                SetState(LinkState::Failed, LinkError::NotPaired,
                         "Pareie seu PS5 primeiro.", attempt);
                break;
            }

            SetState(LinkState::Connecting, LinkError::None, "Conectando\u2026", attempt + 1);
            const ConnectResult result = backend_.Connect(*pairing);

            if (!Wanted()) {
                backend_.Disconnect();
                SetState(LinkState::Idle, LinkError::Cancelled, "Desconectado.", 0);
                break;
            }

            if (result.ok) {
                attempt = 0;
                {
                    std::lock_guard lock(mutex_);
                    padEnabled_ = true;
                }
                SetState(LinkState::Connected, LinkError::None,
                         result.message.empty() ? "Conectado." : result.message, 0);
                HoldWhileConnected();
                {
                    std::lock_guard lock(mutex_);
                    padEnabled_ = false;
                }
                backend_.Disconnect();

                if (!Wanted()) {
                    SetState(LinkState::Idle, LinkError::Cancelled, "Desconectado.", 0);
                    break;
                }
                if (!policy_.autoReconnect) {
                    {
                        std::lock_guard lock(mutex_);
                        wantConnected_ = false;
                    }
                    SetState(LinkState::Failed, LinkError::Protocol,
                             "A conexão com o PS5 caiu. Pressione Jogar para conectar de novo.", 0);
                    break;
                }
                SetState(LinkState::Retrying, LinkError::Protocol,
                         "A conexão caiu. Tentando reconectar\u2026", 1);
                WaitRetry(policy_.baseDelayMs);
                if (!Wanted()) {
                    SetState(LinkState::Idle, LinkError::Cancelled, "Desconectado.", 0);
                    break;
                }
                continue;
            }

            if (IsFatal(result.error)) {
                {
                    std::lock_guard lock(mutex_);
                    wantConnected_ = false;
                }
                SetState(LinkState::Failed, result.error, result.message, attempt + 1);
                break;
            }

            ++attempt;
            unsigned int delay = policy_.baseDelayMs;
            for (unsigned int i = 1; i < attempt && delay < policy_.maxDelayMs; ++i) delay *= 2;
            delay = std::min(delay, policy_.maxDelayMs);
            if (result.error == LinkError::Refused) delay = std::max(delay, policy_.refusedDelayMs);

            SetState(LinkState::Retrying, result.error, result.message, attempt);
            WaitRetry(delay);

            if (!Wanted()) {
                SetState(LinkState::Idle, LinkError::Cancelled, "Desconectado.", 0);
                break;
            }
        }
    }
}

}  // namespace pskb
