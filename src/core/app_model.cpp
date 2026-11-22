#include "core/app_model.h"

#include <cctype>
#include <cwctype>
#include <filesystem>
#include <iterator>

namespace pskb {

const char* AppStatusText(AppStatus status) noexcept {
    switch (status) {
        case AppStatus::Disconnected: return "Desconectado";
        case AppStatus::Connecting: return "Conectando";
        case AppStatus::Connected: return "Conectado";
        case AppStatus::Playing: return "Jogando";
        case AppStatus::NoNetwork: return "Sem rede";
        case AppStatus::Error: return "Erro";
    }
    return "Desconectado";
}

AppStatus DeriveAppStatus(LinkState link, CaptureMode capture, LinkError error) noexcept {
    switch (link) {
        case LinkState::Failed:
            return AppStatus::Error;
        case LinkState::Connecting:
            return AppStatus::Connecting;
        case LinkState::Retrying:
            return error == LinkError::NetworkUnavailable ? AppStatus::NoNetwork : AppStatus::Connecting;
        case LinkState::Connected:
            return capture == CaptureMode::Active ? AppStatus::Playing : AppStatus::Connected;
        case LinkState::Idle:
            return AppStatus::Disconnected;
    }
    return AppStatus::Disconnected;
}

bool CanAdvance(WizardStep step, const WizardProgress& progress) noexcept {
    switch (step) {
        case WizardStep::Console: return progress.consoleChosen;
        case WizardStep::Account: return progress.accountReady;
        case WizardStep::Pin: return progress.paired;
        case WizardStep::Test: return progress.connectionTested;
        case WizardStep::Welcome:
        case WizardStep::RemotePlay:
        case WizardStep::Veyra:
        case WizardStep::Done:
            return true;
    }
    return false;
}

WizardStep NextWizardStep(WizardStep step) noexcept {
    const auto index = static_cast<unsigned char>(step);
    if (index + 1 >= kWizardStepCount) return WizardStep::Done;
    return static_cast<WizardStep>(index + 1);
}

WizardStep PreviousWizardStep(WizardStep step) noexcept {
    const auto index = static_cast<unsigned char>(step);
    if (index == 0) return WizardStep::Welcome;
    return static_cast<WizardStep>(index - 1);
}

std::size_t WizardStepNumber(WizardStep step) noexcept {
    return static_cast<std::size_t>(step) + 1;
}

const char* WizardStepTitle(WizardStep step) noexcept {
    switch (step) {
        case WizardStep::Welcome: return "Bem-vindo";
        case WizardStep::Console: return "Encontrar o PS5";
        case WizardStep::RemotePlay: return "Ativar o Remote Play";
        case WizardStep::Account: return "Conta PSN";
        case WizardStep::Pin: return "Parear com o PS5";
        case WizardStep::Test: return "Testar a conexão";
        case WizardStep::Veyra: return "Veyra";
        case WizardStep::Done: return "Pronto para jogar";
    }
    return "";
}

std::optional<std::string> NormalizePin(std::string_view text) {
    std::string digits;
    for (const char c : text) {
        if (c == ' ' || c == '-' || c == '\t') continue;
        if (!std::isdigit(static_cast<unsigned char>(c))) return std::nullopt;
        digits.push_back(c);
    }
    if (digits.size() != 8) return std::nullopt;
    return digits;
}

bool IsVeyraExecutableName(std::wstring_view fileName) noexcept {
    if (fileName.size() != std::size(kVeyraExecutableName) - 1) return false;
    for (std::size_t i = 0; i < fileName.size(); ++i) {
        const wchar_t expected = kVeyraExecutableName[i];
        const wchar_t actual = fileName[i];
        if (std::towlower(actual) != std::towlower(expected)) return false;
    }
    return true;
}

std::optional<std::wstring> PickVeyraExecutable(const std::vector<std::wstring>& candidates) {
    for (const auto& candidate : candidates) {
        const std::wstring name = std::filesystem::path(candidate).filename().wstring();
        if (IsVeyraExecutableName(name)) return candidate;
    }
    return std::nullopt;
}

}  // namespace pskb
