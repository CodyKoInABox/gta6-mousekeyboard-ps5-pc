#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "core/capture_session.h"
#include "core/link_manager.h"

namespace pskb {

// What the main screen shows. Derived from the link and capture state, never stored.
enum class AppStatus : unsigned char { Disconnected, Connecting, Connected, Playing, NoNetwork, Error };

const char* AppStatusText(AppStatus status) noexcept;
// `error` is the link's current error; a Retrying link with NetworkUnavailable shows NoNetwork.
AppStatus DeriveAppStatus(LinkState link, CaptureMode capture, LinkError error = LinkError::None) noexcept;

enum class WizardStep : unsigned char {
    Welcome,
    Console,
    RemotePlay,
    Account,
    Pin,
    Test,
    Veyra,
    Done,
};

inline constexpr std::size_t kWizardStepCount = 8;

// What the wizard has achieved so far. Each step may only be left once its requirement holds,
// so the user can never reach the PIN step without a console and an account.
struct WizardProgress {
    bool consoleChosen = false;
    bool accountReady = false;
    bool paired = false;
    bool connectionTested = false;
};

bool CanAdvance(WizardStep step, const WizardProgress& progress) noexcept;
WizardStep NextWizardStep(WizardStep step) noexcept;
WizardStep PreviousWizardStep(WizardStep step) noexcept;
std::size_t WizardStepNumber(WizardStep step) noexcept;
const char* WizardStepTitle(WizardStep step) noexcept;

// Accepts 8 digits, optionally grouped with spaces or hyphens ("1234 5678"). Returns the digits.
std::optional<std::string> NormalizePin(std::string_view text);

inline constexpr wchar_t kVeyraExecutableName[] = L"Veyra.exe";
bool IsVeyraExecutableName(std::wstring_view fileName) noexcept;
// First candidate whose file name is Veyra.exe. Candidates are ordered by preference by the caller.
std::optional<std::wstring> PickVeyraExecutable(const std::vector<std::wstring>& candidates);

}  // namespace pskb
