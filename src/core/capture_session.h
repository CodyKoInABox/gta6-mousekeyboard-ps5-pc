#pragma once

#include "core/input_codes.h"
#include "core/pad_engine.h"
#include "core/pad_state.h"

namespace pskb {

// Off:    not capturing. Nothing reaches the pad.
// Armed:  the user turned capture on, but the approved window is not focused or the link is not
//         ready. Nothing reaches the pad, and held inputs are released.
// Active: capture is on, the approved window has focus, and the link is ready. Keys and mouse
//         drive the pad.
enum class CaptureMode : unsigned char { Off, Armed, Active };

const char* CaptureModeName(CaptureMode mode) noexcept;

// The gating rules for input capture. Pure logic with no Windows calls, so it can be tested
// without a desktop. The Windows layer feeds it raw events and reports focus and link changes.
//
// Safety rules:
//  * Leaving Active (focus lost, link lost, disarm) releases every held input and recentres
//    both sticks at once.
//  * Key-up is always forwarded, so a key pressed while active cannot stay stuck.
//  * Esc disarms whenever capture is on. The hotkey toggles and is never forwarded as a game key.
//  * Nothing is typed or injected into other applications. Events are observed only.
class CaptureSession {
public:
    static constexpr InputCode kEscape = 0x1B;

    explicit CaptureSession(PadEngine& engine, InputCode hotkey = 0x77);

    CaptureMode Mode() const noexcept { return mode_; }

    void SetHotkey(InputCode code) noexcept { hotkey_ = code; }

    void Toggle();
    void Disarm();

    void SetGameFocused(bool focused);
    void SetLinkReady(bool ready);

    void OnKey(InputCode code, bool down);
    void OnMouseButton(InputCode code, bool down);
    void OnMouseMove(int dx, int dy, double nowMs);

    PadState Tick(double nowMs);

private:
    void Recompute();

    PadEngine& engine_;
    InputCode hotkey_;
    CaptureMode mode_ = CaptureMode::Off;
    bool armed_ = false;
    bool gameFocused_ = false;
    bool linkReady_ = false;
    bool hotkeyDown_ = false;
};

}  // namespace pskb
