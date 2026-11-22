#include "core/input_gate.h"

namespace pskb {
namespace {

constexpr InputCode kLeftWin = 0x5B;
constexpr InputCode kRightWin = 0x5C;

}  // namespace

bool ShouldSwallow(const GateEvent& event, CaptureMode mode, bool approvedFocused) noexcept {
    if (mode != CaptureMode::Active || !approvedFocused) return false;
    if (event.injected || event.reserved) return false;
    if (event.winHeld || event.code == kLeftWin || event.code == kRightWin) return false;
    if (event.altHeld) return false;

    if (!event.keyboard) return true;

    if (event.code == CaptureSession::kEscape) return !event.ctrlHeld;
    return event.mapped;
}

}  // namespace pskb
