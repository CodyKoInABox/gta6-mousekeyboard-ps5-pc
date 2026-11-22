#pragma once

#include <string>

#include "core/pad_state.h"
#include "core/pairing.h"

namespace pskb {

// Why a connection attempt or session ended. Drives retry policy and the message shown to the user.
enum class LinkError : unsigned char {
    None,
    NotPaired,        // no saved pairing: fatal until the user pairs again
    NotFound,         // PS5 not seen on the LAN yet: retry
    Asleep,           // PS5 in rest mode and did not wake: retry
    Refused,          // PS5 refused the session (often just after a previous one): retry later
    LoginPasscode,    // PS5 asks for a user passcode, which this app cannot enter: fatal
    NetworkUnavailable,
    Protocol,         // unexpected protocol failure: retry with backoff
    Cancelled,        // the user stopped the connection
    Other,
};

bool IsFatal(LinkError error) noexcept;
const char* LinkErrorName(LinkError error) noexcept;

struct ConnectResult {
    bool ok = false;
    LinkError error = LinkError::Other;
    std::string message;  // plain-language text, safe to show to the user
};

// Transport to the PS5. The real implementation wraps chiaki-ng (input-only Remote Play).
// Implementations must be safe to call from two threads: one drives Connect(), the input
// thread calls SendPad().
class IRemotePlayBackend {
public:
    virtual ~IRemotePlayBackend() = default;

    // Blocks until the session is up, fails, or RequestCancel() is called.
    virtual ConnectResult Connect(const PairingInfo& pairing) = 0;

    // Sends one controller frame. Silently ignored when not connected.
    virtual void SendPad(const PadState& pad) = 0;

    virtual bool IsConnected() const = 0;

    // Ends the session. Must send a neutral pad first. Idempotent.
    virtual void Disconnect() = 0;

    // Makes a blocking Connect() return early. Safe to call from any thread.
    virtual void RequestCancel() = 0;
};

}  // namespace pskb
