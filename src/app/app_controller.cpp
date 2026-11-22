#include "app/app_controller.h"

#include "app/saved_pairing.h"
#include "app/ui.h"
#include "core/app_paths.h"
#include "platform/network_status.h"
#include "platform/veyra_locator.h"

namespace pskb {

AppController::AppController() {
    LoadSettingsFile(SettingsFilePath(), settings_);
}

AppController::~AppController() { StopPlay(); }

bool AppController::SaveSettings() const {
    return SaveSettingsFile(SettingsFilePath(), settings_);
}

bool AppController::HasPairing() const { return LoadSavedPairing().has_value(); }

Profile AppController::ActiveProfile() const {
    const auto saved = settings_.savedBindings.find(settings_.profileId);
    const std::string savedText = saved == settings_.savedBindings.end() ? std::string{} : saved->second;
    Profile profile = ProfileWithBindings(settings_.profileId, savedText);
    profile.aim = settings_.aim;
    return profile;
}

bool AppController::StartPlay(std::string* error) {
    if (IsPlaying()) return true;
    if (!HasPairing()) {
        if (error) *error = "Nenhum PS5 pareado. Use a configuração inicial primeiro.";
        return false;
    }

    if (settings_.launchVeyra) {
        if (settings_.veyraPath.empty()) {
            if (const auto found = FindVeyraExecutable()) {
                settings_.veyraPath = ui::ToUtf8(*found);
                SaveSettings();
            }
        }
        if (!settings_.veyraPath.empty()) LaunchOrFocusVeyra(ui::ToWide(settings_.veyraPath));
    }

    backend_ = std::make_unique<ChiakiBackend>();
    RetryPolicy policy;
    policy.autoReconnect = settings_.autoReconnect;
    link_ = std::make_unique<LinkManager>(*backend_, [] { return LoadSavedPairing(); }, policy,
                                          [] { return HasUsableNetwork(); });

    LinkManager* link = link_.get();
    CaptureOptions options;
    options.hotkey = static_cast<InputCode>(settings_.captureHotkeyVk);
    bridge_ = std::make_unique<InputBridge>(ActiveProfile(), options,
                                            [link](const PadState& pad) { link->SubmitPad(pad); });

    std::string bridgeError;
    if (!bridge_->Start(&bridgeError)) {
        bridge_.reset();
        link_.reset();
        backend_.reset();
        if (error) *error = bridgeError;
        return false;
    }

    linkReady_ = false;
    bridge_->Toggle();
    link_->Connect();
    return true;
}

void AppController::StopPlay() {
    // The capture thread feeds the link, so it must stop before the link is torn down.
    bridge_.reset();
    if (link_) link_->Disconnect();
    link_.reset();
    backend_.reset();
    linkReady_ = false;
}

bool AppController::IsPlaying() const { return link_ != nullptr || bridge_ != nullptr; }

void AppController::Tick() {
    if (!bridge_ || !link_) return;
    const bool ready = link_->IsConnected();
    if (ready != linkReady_) {
        linkReady_ = ready;
        bridge_->SetLinkReady(ready);
    }
}

AppStatus AppController::Status() const {
    const LinkStatus link = link_ ? link_->Status() : LinkStatus{};
    return DeriveAppStatus(link.state, Capture(), link.error);
}

std::string AppController::StatusMessage() const {
    return link_ ? link_->Status().message : std::string{};
}

CaptureMode AppController::Capture() const { return bridge_ ? bridge_->Mode() : CaptureMode::Off; }

bool AppController::CaptureBlocking() const { return bridge_ && bridge_->SuppressionInstalled(); }

}  // namespace pskb
