#include <string>
#include <vector>

#include "core/app_model.h"
#include "test_framework.h"

using namespace pskb;

namespace {

std::string Status(AppStatus status) { return AppStatusText(status); }

}  // namespace

PSKB_TEST(StatusTextIsPortugueseAndExact) {
    CHECK_EQ(Status(AppStatus::Disconnected), std::string("Desconectado"));
    CHECK_EQ(Status(AppStatus::Connecting), std::string("Conectando"));
    CHECK_EQ(Status(AppStatus::Connected), std::string("Conectado"));
    CHECK_EQ(Status(AppStatus::Playing), std::string("Jogando"));
    CHECK_EQ(Status(AppStatus::Error), std::string("Erro"));
}

PSKB_TEST(IdleLinkIsDisconnectedWhateverCaptureDoes) {
    CHECK(DeriveAppStatus(LinkState::Idle, CaptureMode::Off) == AppStatus::Disconnected);
}

PSKB_TEST(ConnectingAndRetryingShowConnecting) {
    CHECK(DeriveAppStatus(LinkState::Connecting, CaptureMode::Off) == AppStatus::Connecting);
    CHECK(DeriveAppStatus(LinkState::Retrying, CaptureMode::Armed) == AppStatus::Connecting);
}

PSKB_TEST(ConnectedWithoutActiveCaptureIsConnected) {
    CHECK(DeriveAppStatus(LinkState::Connected, CaptureMode::Off) == AppStatus::Connected);
    CHECK(DeriveAppStatus(LinkState::Connected, CaptureMode::Armed) == AppStatus::Connected);
}

PSKB_TEST(ConnectedAndActiveIsPlaying) {
    CHECK(DeriveAppStatus(LinkState::Connected, CaptureMode::Active) == AppStatus::Playing);
}

PSKB_TEST(FailedLinkIsErrorWhateverCaptureDoes) {
    CHECK(DeriveAppStatus(LinkState::Failed, CaptureMode::Off) == AppStatus::Error);
    CHECK(DeriveAppStatus(LinkState::Failed, CaptureMode::Armed) == AppStatus::Error);
}

PSKB_TEST(WizardStepsNavigateInOrderAndStopAtTheEnds) {
    WizardStep step = WizardStep::Welcome;
    std::size_t visited = 1;
    while (step != WizardStep::Done) {
        step = NextWizardStep(step);
        ++visited;
    }
    CHECK_EQ(visited, kWizardStepCount);
    CHECK(NextWizardStep(WizardStep::Done) == WizardStep::Done);
    CHECK(PreviousWizardStep(WizardStep::Welcome) == WizardStep::Welcome);
    CHECK(PreviousWizardStep(WizardStep::Pin) == WizardStep::Account);
    CHECK_EQ(WizardStepNumber(WizardStep::Welcome), static_cast<std::size_t>(1));
    CHECK_EQ(WizardStepNumber(WizardStep::Done), kWizardStepCount);
}

PSKB_TEST(WizardStepTitlesArePortuguese) {
    CHECK_EQ(std::string(WizardStepTitle(WizardStep::Pin)), std::string("Parear com o PS5"));
    CHECK_EQ(std::string(WizardStepTitle(WizardStep::Done)), std::string("Pronto para jogar"));
}

PSKB_TEST(ConsoleStepNeedsAChosenConsole) {
    WizardProgress progress;
    CHECK(!CanAdvance(WizardStep::Console, progress));
    progress.consoleChosen = true;
    CHECK(CanAdvance(WizardStep::Console, progress));
}

PSKB_TEST(AccountStepNeedsARecognisedAccount) {
    WizardProgress progress;
    CHECK(!CanAdvance(WizardStep::Account, progress));
    progress.accountReady = true;
    CHECK(CanAdvance(WizardStep::Account, progress));
}

PSKB_TEST(PinStepCannotBeLeftBeforePairing) {
    WizardProgress progress;
    progress.consoleChosen = true;
    progress.accountReady = true;
    CHECK(!CanAdvance(WizardStep::Pin, progress));
    progress.paired = true;
    CHECK(CanAdvance(WizardStep::Pin, progress));
}

PSKB_TEST(TestStepNeedsASuccessfulConnection) {
    WizardProgress progress;
    CHECK(!CanAdvance(WizardStep::Test, progress));
    progress.connectionTested = true;
    CHECK(CanAdvance(WizardStep::Test, progress));
}

PSKB_TEST(InformationalStepsAlwaysAdvance) {
    WizardProgress none;
    CHECK(CanAdvance(WizardStep::Welcome, none));
    CHECK(CanAdvance(WizardStep::RemotePlay, none));
    CHECK(CanAdvance(WizardStep::Veyra, none));
    CHECK(CanAdvance(WizardStep::Done, none));
}

PSKB_TEST(PinAcceptsEightDigitsWithOptionalGrouping) {
    CHECK(NormalizePin("12345678") == std::optional<std::string>("12345678"));
    CHECK(NormalizePin("1234 5678") == std::optional<std::string>("12345678"));
    CHECK(NormalizePin("1234-5678") == std::optional<std::string>("12345678"));
    CHECK(NormalizePin("  1234 5678 ") == std::optional<std::string>("12345678"));
}

PSKB_TEST(PinRejectsWrongLengthAndLetters) {
    CHECK(!NormalizePin("").has_value());
    CHECK(!NormalizePin("1234567").has_value());
    CHECK(!NormalizePin("123456789").has_value());
    CHECK(!NormalizePin("12345678a").has_value());
    CHECK(!NormalizePin("1234.5678").has_value());
}

PSKB_TEST(VeyraExecutableNameMatchesCaseInsensitively) {
    CHECK(IsVeyraExecutableName(L"Veyra.exe"));
    CHECK(IsVeyraExecutableName(L"VEYRA.EXE"));
    CHECK(!IsVeyraExecutableName(L"Veyra"));
    CHECK(!IsVeyraExecutableName(L"Veyra.dll"));
    CHECK(!IsVeyraExecutableName(L"Veyra.exe.bak"));
    CHECK(!IsVeyraExecutableName(L"NotVeyra.exe"));
}

PSKB_TEST(PickVeyraSkipsOtherFilesAndReturnsTheExecutable) {
    const std::vector<std::wstring> candidates = {
        L"C:\\Users\\me\\notes.txt",
        L"D:\\Games\\Veyra\\Veyra.exe",
        L"C:\\Other\\Veyra.exe",
    };
    const auto picked = PickVeyraExecutable(candidates);
    CHECK(picked.has_value());
    CHECK(*picked == L"D:\\Games\\Veyra\\Veyra.exe");
}

PSKB_TEST(PickVeyraReturnsNothingWithoutAMatch) {
    CHECK(!PickVeyraExecutable({L"C:\\a\\b.exe", L"C:\\c\\Veyra.dll"}).has_value());
    CHECK(!PickVeyraExecutable({}).has_value());
}
