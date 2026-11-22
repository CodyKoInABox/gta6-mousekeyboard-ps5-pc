#include "core/capture_session.h"

namespace pskb {

const char* CaptureModeName(CaptureMode mode) noexcept {
    switch (mode) {
        case CaptureMode::Off: return "off";
        case CaptureMode::Armed: return "armed";
        case CaptureMode::Active: return "active";
    }
    return "off";
}

CaptureSession::CaptureSession(PadEngine& engine, InputCode hotkey) : engine_(engine), hotkey_(hotkey) {}

void CaptureSession::Toggle() {
    if (armed_) {
        Disarm();
    } else {
        armed_ = true;
        Recompute();
    }
}

void CaptureSession::Disarm() {
    armed_ = false;
    Recompute();
}

void CaptureSession::SetGameFocused(bool focused) {
    gameFocused_ = focused;
    Recompute();
}

void CaptureSession::SetLinkReady(bool ready) {
    linkReady_ = ready;
    Recompute();
}

void CaptureSession::Recompute() {
    CaptureMode next = CaptureMode::Off;
    if (armed_) next = (gameFocused_ && linkReady_) ? CaptureMode::Active : CaptureMode::Armed;

    if (mode_ == CaptureMode::Active && next != CaptureMode::Active) {
        engine_.ReleaseAll();
    }
    if (next == CaptureMode::Off) {
        engine_.ReleaseAll();
    }
    mode_ = next;
}

void CaptureSession::OnKey(InputCode code, bool down) {
    if (code == hotkey_) {
        if (!down) {
            hotkeyDown_ = false;
        } else if (!hotkeyDown_) {
            hotkeyDown_ = true;
            Toggle();
        }
        return;
    }

    if (!down) {
        engine_.KeyUp(code);
        return;
    }

    if (code == kEscape && armed_) {
        Disarm();
        return;
    }
    if (mode_ == CaptureMode::Active) engine_.KeyDown(code);
}

void CaptureSession::OnMouseButton(InputCode code, bool down) {
    if (!down) {
        engine_.KeyUp(code);
        return;
    }
    if (mode_ == CaptureMode::Active) engine_.KeyDown(code);
}

void CaptureSession::OnMouseMove(int dx, int dy, double nowMs) {
    if (mode_ == CaptureMode::Active) engine_.MouseMove(dx, dy, nowMs);
}

PadState CaptureSession::Tick(double nowMs) {
    return engine_.Update(nowMs);
}

}  // namespace pskb
