#include "app/app_controller.h"

#include "app/saved_pairing.h"
#include "app/ui.h"
#include "core/app_paths.h"
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
    Profile profile = settings_.profileId == "driving" ? DrivingProfile() : GtaProfile();
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
    link_ = std::make_unique<LinkManager>(*backend_, [] { return LoadSavedPairing(); }, policy);

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
    const LinkState link = link_ ? link_->Status().state : LinkState::Idle;
    return DeriveAppStatus(link, Capture());
}

std::string AppController::StatusMessage() const {
    return link_ ? link_->Status().message : std::string{};
}

CaptureMode AppController::Capture() const { return bridge_ ? bridge_->Mode() : CaptureMode::Off; }

bool AppController::CaptureBlocking() const { return bridge_ && bridge_->SuppressionInstalled(); }

}  // namespace pskb
