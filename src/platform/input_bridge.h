#pragma once

#include <windows.h>

#include <atomic>
#include <functional>
#include <future>
#include <string>
#include <thread>
#include <vector>

#include "core/capture_session.h"
#include "core/input_codes.h"
#include "core/pad_engine.h"
#include "core/pad_state.h"

namespace pskb {

struct CaptureOptions {
    std::wstring approvedProcess = L"Veyra";  // executable name, with or without ".exe"
    InputCode hotkey = 0x77;                  // F8
};

// Observes the keyboard and mouse with Raw Input and turns them into pad frames while the
// approved window has focus. Events are read, never consumed or injected, so nothing is typed
// into other applications and the Windows cursor and shortcuts keep working.
//
// Runs on its own thread. Toggle, Disarm and SetLinkReady are thread-safe and take effect on
// that thread. Stop (and the destructor) releases every input before returning.
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

private:
    void Run(std::promise<std::string>& ready);
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void Tick();
    void OnRawInput(LPARAM lParam);
    bool IsApprovedWindow(HWND window) const;
    void PostToWindow(UINT msg, WPARAM wParam) const;

    CaptureOptions options_;
    PadSink sink_;
    ModeListener onMode_;
    PadEngine engine_;
    CaptureSession session_;

    std::atomic<CaptureMode> mode_{CaptureMode::Off};
    std::atomic<bool> running_{false};
    HWND hwnd_ = nullptr;
    HWND lastForeground_ = nullptr;
    std::thread thread_;
    std::vector<BYTE> rawBuffer_;
};

}  // namespace pskb
