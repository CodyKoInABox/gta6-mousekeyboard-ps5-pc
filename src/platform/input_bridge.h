#pragma once

#include <windows.h>

#include <atomic>
#include <functional>
#include <future>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include "core/capture_session.h"
#include "core/input_codes.h"
#include "core/input_gate.h"
#include "core/pad_engine.h"
#include "core/pad_state.h"

namespace pskb {

struct CaptureOptions {
    std::wstring approvedProcess = L"Veyra";  // executable name, with or without ".exe"
    InputCode hotkey = 0x77;                  // F8
};

// Observes the keyboard and mouse with Raw Input and turns them into pad frames while the
// approved window has focus. Raw Input never consumes or injects anything.
//
// While capture is Active and the approved window has focus, scoped low-level hooks swallow the
// mapped keys and mouse buttons/wheel so Veyra never sees them, and the cursor is clipped to
// Veyra's window and hidden. The hooks and the clip are removed as soon as capture leaves
// Active, and Stop (and the destructor) removes them before returning.
//
// Runs on its own thread. Toggle, Disarm and SetLinkReady are thread-safe and take effect on
// that thread.
class InputBridge {
public:
    using PadSink = std::function<void(const PadState&)>;
    using ModeListener = std::function<void(CaptureMode)>;

    InputBridge(const Profile& profile, CaptureOptions options, PadSink sink, ModeListener onMode = {});
    ~InputBridge();

    InputBridge(const InputBridge&) = delete;
    InputBridge& operator=(const InputBridge&) = delete;

    // Returns false and fills error (plain text) when the window or Raw Input cannot be set up.
    bool Start(std::string* error);
    void Stop();

    void Toggle();
    void Disarm();
    void SetLinkReady(bool ready);

    CaptureMode Mode() const noexcept { return mode_.load(); }

    // True while the swallowing hooks are installed (only ever true while Active).
    bool SuppressionInstalled() const noexcept { return suppressionInstalled_.load(); }

private:
    void Run(std::promise<std::string>& ready);
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK KeyboardHookProc(int code, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK MouseHookProc(int code, WPARAM wParam, LPARAM lParam);
    void Tick();
    void OnRawInput(LPARAM lParam);
    void RefreshForeground();
    bool IsApprovedWindow(HWND window) const;
    void PostToWindow(UINT msg, WPARAM wParam) const;

    bool OnKeyboardHook(const KBDLLHOOKSTRUCT& key, WPARAM message);
    bool OnMouseHook(const MSLLHOOKSTRUCT& mouse, WPARAM message);
    GateEvent MakeGateEvent(InputCode code, bool down, bool keyboard, bool injected) const;
    void SyncSuppression();
    void RemoveSuppression();
    void ApplyCursorLock(HWND window);
    void ReleaseCursorLock();

    CaptureOptions options_;
    PadSink sink_;
    ModeListener onMode_;
    PadEngine engine_;
    CaptureSession session_;
    std::unordered_set<InputCode> mapped_;
    SwallowedKeys swallowed_;

    std::atomic<CaptureMode> mode_{CaptureMode::Off};
    std::atomic<bool> running_{false};
    std::atomic<bool> suppressionInstalled_{false};
    HWND hwnd_ = nullptr;
    HWND lastForeground_ = nullptr;
    bool approved_ = false;
    HHOOK keyHook_ = nullptr;
    HHOOK mouseHook_ = nullptr;
    bool clipped_ = false;
    bool cursorHidden_ = false;
    RECT clipRect_{};
    std::thread thread_;
    std::vector<BYTE> rawBuffer_;
};

}  // namespace pskb
