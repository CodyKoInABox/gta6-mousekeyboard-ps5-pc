#pragma once

#include <memory>
#include <string>

#include "backend/chiaki_backend.h"
#include "core/app_model.h"
#include "core/bindings.h"
#include "core/link_manager.h"
#include "core/settings.h"
#include "platform/input_bridge.h"

namespace pskb {

// Owns the Remote Play link and the input capture for the GUI. Everything here runs on the UI
// thread; the capture thread only calls into the link through SubmitPad.
class AppController {
public:
    AppController();
    ~AppController();

    AppController(const AppController&) = delete;
    AppController& operator=(const AppController&) = delete;

    AppSettings& Settings() noexcept { return settings_; }
    const AppSettings& Settings() const noexcept { return settings_; }
    bool SaveSettings() const;

    bool HasPairing() const;

    // Connects to the PS5 and arms capture. Capture becomes Playing once Veyra has focus.
    bool StartPlay(std::string* error);
    void StopPlay();
    bool IsPlaying() const;

    // Pushes link readiness into the capture. Call on the UI thread, about four times a second.
    void Tick();

    AppStatus Status() const;
    std::string StatusMessage() const;
    CaptureMode Capture() const;
    // False when the blocking hooks could not be installed. Keys then still reach Veyra.
    bool CaptureBlocking() const;

private:
    Profile ActiveProfile() const;

    AppSettings settings_;
    std::unique_ptr<ChiakiBackend> backend_;
    std::unique_ptr<LinkManager> link_;
    std::unique_ptr<InputBridge> bridge_;
    bool linkReady_ = false;
};

}  // namespace pskb
