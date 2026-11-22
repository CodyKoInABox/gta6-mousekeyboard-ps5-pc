#include "app/settings_window.h"

#include <commdlg.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <utility>

#include "app/saved_pairing.h"
#include "app/ui.h"
#include "core/input_codes.h"
#include "platform/veyra_locator.h"

namespace pskb {
namespace {

constexpr wchar_t kClassName[] = L"PS5KeyboardBridge.Settings";
constexpr int kWidth = 760;
constexpr int kHeightCollapsed = 690;
constexpr int kHeightExpanded = 900;
constexpr int kBottomCollapsedY = 590;
constexpr int kBottomExpandedY = 794;
constexpr int kMessageGap = 56;
constexpr DWORD kStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

enum Id : int {
    kTitle = 100,
    kProfileLabel,
    kProfile,
    kVeyraLabel,
    kVeyraPath,
    kVeyraBrowse,
    kVeyraDetect,
    kHotkeyLabel,
    kHotkey,
    kBindings,
    kAutoConnect,
    kAutoReconnect,
    kMinimizeToTray,
    kLaunchVeyra,
    kAdvanced,
    kSensXLabel,
    kSensX,
    kSensYLabel,
    kSensY,
    kAimSensXLabel,
    kAimSensX,
    kAimSensYLabel,
    kAimSensY,
    kDeadzoneLabel,
    kDeadzone,
    kAntiDeadzoneLabel,
    kAntiDeadzone,
    kAccelerationLabel,
    kAcceleration,
    kInvertY,
    kSmoothLabel,
    kSmooth,
    kCurveExpLabel,
    kCurveExp,
    kCurve,
    kAimNote,
    kForget,
    kSetup,
    kCancel,
    kSave,
    kMessage,
};

constexpr int kProfileGta = 0;
constexpr int kProfileDriving = 1;

std::string Format(float value, int decimals = 2) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.*f", decimals, static_cast<double>(value));
    return buffer;
}

std::optional<float> ParseFloat(std::string text, float low, float high) {
    for (char& c : text) {
        if (c == ',') c = '.';
    }
    char* end = nullptr;
    const float value = std::strtof(text.c_str(), &end);
    if (end == text.c_str() || *end != '\0' || !std::isfinite(value)) return std::nullopt;
    if (value < low || value > high) return std::nullopt;
    return value;
}

void MoveToY(HWND parent, int id, int y) {
    HWND control = GetDlgItem(parent, id);
    RECT rect{};
    GetWindowRect(control, &rect);
    MapWindowPoints(nullptr, parent, reinterpret_cast<POINT*>(&rect), 2);
    SetWindowPos(control, nullptr, rect.left, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

}  // namespace

SettingsWindow::SettingsWindow(HINSTANCE instance, AppController& app, std::function<void()> onClosed,
                               std::function<void()> onSetup)
    : instance_(instance), app_(app), onClosed_(std::move(onClosed)), onSetup_(std::move(onSetup)) {
    Create(instance);
    Load();
    Show();
}

SettingsWindow::~SettingsWindow() {
    bindings_.reset();
    if (hwnd_) DestroyWindow(hwnd_);
}

void SettingsWindow::Show() {
    ShowWindow(hwnd_, SW_SHOW);
    SetForegroundWindow(hwnd_);
}

void SettingsWindow::Create(HINSTANCE instance) {
    static const bool registered = [instance] {
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = &SettingsWindow::WndProc;
        wc.hInstance = instance;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = ui::WindowBackground();
        wc.lpszClassName = kClassName;
        return RegisterClassExW(&wc) != 0;
    }();
    if (!registered) return;

    RECT frame{0, 0, kWidth, kHeightCollapsed};
    AdjustWindowRect(&frame, kStyle, FALSE);
    hwnd_ = CreateWindowExW(WS_EX_CONTROLPARENT, kClassName, L"Configurações - PS5 Keyboard Bridge", kStyle,
                            CW_USEDEFAULT, CW_USEDEFAULT, frame.right - frame.left, frame.bottom - frame.top,
                            nullptr, nullptr, instance, this);

    ui::Label(hwnd_, "Configurações", kTitle, 40, 24, 680, 36, ui::Font(26, FW_BOLD));

    ui::Label(hwnd_, "Perfil de controle", kProfileLabel, 40, 86, 200, 24, ui::Font(18));
    HWND profile = ui::ComboBox(hwnd_, kProfile, 260, 82, 440, 200);
    SendMessageW(profile, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"GTA 6 (padrão)"));
    SendMessageW(profile, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Dirigir (volante suave)"));

    ui::Label(hwnd_, "Caminho do Veyra.exe (vazio = detectar sozinho)", kVeyraLabel, 40, 128, 680, 24,
              ui::Font(18));
    ui::Edit(hwnd_, kVeyraPath, 40, 156, 430, 30, 512);
    ui::Button(hwnd_, "Procurar...", kVeyraBrowse, 480, 154, 130, 34);
    ui::Button(hwnd_, "Detectar", kVeyraDetect, 620, 154, 100, 34);

    ui::Label(hwnd_, "Tecla que liga e desliga a captura", kHotkeyLabel, 40, 208, 400, 24, ui::Font(18));
    ui::Edit(hwnd_, kHotkey, 460, 204, 100, 30, 16);
    ui::Button(hwnd_, "Editar teclas...", kBindings, 572, 202, 148, 34);

    ui::Check(hwnd_, "Conectar ao abrir o app", kAutoConnect, 40, 252, 680, 26, false);
    ui::Check(hwnd_, "Reconectar automaticamente se a conexão cair", kAutoReconnect, 40, 286, 680, 26, true);
    ui::Check(hwnd_, "Minimizar para a bandeja ao fechar a janela", kMinimizeToTray, 40, 320, 680, 26, true);
    ui::Check(hwnd_, "Abrir o Veyra ao jogar", kLaunchVeyra, 40, 354, 680, 26, true);

    ui::Check(hwnd_, "Mostrar ajustes avançados de mira (só para quem sabe o que faz)", kAdvanced, 40, 400, 680, 28,
              false);

    advanced_.push_back(ui::Label(hwnd_, "Câmera X", kSensXLabel, 40, 440, 215, 24, ui::Font(18)));
    advanced_.push_back(ui::Edit(hwnd_, kSensX, 260, 436, 120, 30, 12));
    advanced_.push_back(ui::Label(hwnd_, "Câmera Y", kSensYLabel, 400, 440, 210, 24, ui::Font(18)));
    advanced_.push_back(ui::Edit(hwnd_, kSensY, 620, 436, 120, 30, 12));
    advanced_.push_back(ui::Label(hwnd_, "Mira (L2) X", kAimSensXLabel, 40, 480, 215, 24, ui::Font(18)));
    advanced_.push_back(ui::Edit(hwnd_, kAimSensX, 260, 476, 120, 30, 12));
    advanced_.push_back(ui::Label(hwnd_, "Mira (L2) Y", kAimSensYLabel, 400, 480, 210, 24, ui::Font(18)));
    advanced_.push_back(ui::Edit(hwnd_, kAimSensY, 620, 476, 120, 30, 12));
    advanced_.push_back(ui::Label(hwnd_, "Zona morta (0 a 0,5)", kDeadzoneLabel, 40, 520, 215, 24, ui::Font(18)));
    advanced_.push_back(ui::Edit(hwnd_, kDeadzone, 260, 516, 120, 30, 12));
    advanced_.push_back(ui::Label(hwnd_, "Zona anti-morta (0 a 0,5)", kAntiDeadzoneLabel, 400, 520, 210, 24,
                                  ui::Font(18)));
    advanced_.push_back(ui::Edit(hwnd_, kAntiDeadzone, 620, 516, 120, 30, 12));
    advanced_.push_back(ui::Label(hwnd_, "Aceleração (0 a 1)", kAccelerationLabel, 40, 560, 215, 24, ui::Font(18)));
    advanced_.push_back(ui::Edit(hwnd_, kAcceleration, 260, 556, 120, 30, 12));
    advanced_.push_back(ui::Check(hwnd_, "Inverter eixo Y", kInvertY, 400, 558, 240, 26, false));
    advanced_.push_back(ui::Label(hwnd_, "Suavização (ms)", kSmoothLabel, 40, 600, 215, 24, ui::Font(18)));
    advanced_.push_back(ui::Edit(hwnd_, kSmooth, 260, 596, 120, 30, 12));
    advanced_.push_back(ui::Label(hwnd_, "Expoente da curva (1 a 3)", kCurveExpLabel, 400, 600, 210, 24, ui::Font(18)));
    advanced_.push_back(ui::Edit(hwnd_, kCurveExp, 620, 596, 120, 30, 12));
    advanced_.push_back(ui::Check(hwnd_, "Curva exponencial: mais precisa perto do centro", kCurve, 40, 640, 680, 26,
                                  false));
    advanced_.push_back(ui::Label(
        hwnd_,
        "O mouse vira o analógico direito. Esse analógico tem limite: passada a inclinação máxima, mover mais "
        "rápido não inclina mais, e alguns jogos também limitam a velocidade de giro. Se a câmera parecer lenta "
        "ou rápida demais, ajuste a sensibilidade da câmera. A mira (L2) usa a sensibilidade de mira enquanto você "
        "segura o botão de mira (botão direito no perfil GTA 6).",
        kAimNote, 40, 676, 680, 110, ui::Font(16), ui::kMuted));

    ui::Button(hwnd_, "Esquecer PS5", kForget, 40, kBottomCollapsedY, 170, 44);
    ui::Button(hwnd_, "Refazer configuração inicial", kSetup, 222, kBottomCollapsedY, 262, 44);
    ui::Button(hwnd_, "Cancelar", kCancel, 500, kBottomCollapsedY, 116, 44);
    ui::Button(hwnd_, "Salvar", kSave, 626, kBottomCollapsedY, 114, 44, true);
    ui::Label(hwnd_, "", kMessage, 40, kBottomCollapsedY + kMessageGap, 680, 24, ui::Font(17), ui::kMuted);

    ApplyLayout();
}

LRESULT CALLBACK SettingsWindow::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
    }
    auto* self = reinterpret_cast<SettingsWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self) return self->OnMessage(hwnd, message, wParam, lParam);
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT SettingsWindow::OnMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_COMMAND:
            OnCommand(LOWORD(wParam), HIWORD(wParam));
            return 0;
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORBTN: {
            LRESULT result = 0;
            if (ui::HandleControlColor(message, wParam, lParam, &result)) return result;
            break;
        }
        case WM_DRAWITEM:
            if (ui::DrawButton(lParam)) return TRUE;
            break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            hwnd_ = nullptr;
            if (onClosed_) onClosed_();
            return 0;
        default:
            break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

void SettingsWindow::Load() {
    const AppSettings& s = app_.Settings();
    SendMessageW(GetDlgItem(hwnd_, kProfile), CB_SETCURSEL, s.profileId == "driving" ? kProfileDriving : kProfileGta, 0);
    ui::SetText(GetDlgItem(hwnd_, kVeyraPath), s.veyraPath);

    const auto hotkey = InputCodeToName(static_cast<InputCode>(s.captureHotkeyVk));
    ui::SetText(GetDlgItem(hwnd_, kHotkey), hotkey);

    SendMessageW(GetDlgItem(hwnd_, kAutoConnect), BM_SETCHECK, s.autoConnectOnLaunch ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(GetDlgItem(hwnd_, kAutoReconnect), BM_SETCHECK, s.autoReconnect ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(GetDlgItem(hwnd_, kMinimizeToTray), BM_SETCHECK, s.minimizeToTray ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(GetDlgItem(hwnd_, kLaunchVeyra), BM_SETCHECK, s.launchVeyra ? BST_CHECKED : BST_UNCHECKED, 0);

    ui::SetText(GetDlgItem(hwnd_, kSensX), Format(s.aim.sensitivityX));
    ui::SetText(GetDlgItem(hwnd_, kSensY), Format(s.aim.sensitivityY));
    ui::SetText(GetDlgItem(hwnd_, kAimSensX), Format(s.aim.aimSensitivityX));
    ui::SetText(GetDlgItem(hwnd_, kAimSensY), Format(s.aim.aimSensitivityY));
    ui::SetText(GetDlgItem(hwnd_, kDeadzone), Format(s.aim.deadzone));
    ui::SetText(GetDlgItem(hwnd_, kAntiDeadzone), Format(s.aim.antiDeadzone));
    ui::SetText(GetDlgItem(hwnd_, kAcceleration), Format(s.aim.acceleration));
    ui::SetText(GetDlgItem(hwnd_, kSmooth), Format(s.aim.smoothingMs, 0));
    ui::SetText(GetDlgItem(hwnd_, kCurveExp), Format(s.aim.curveExponent));
    SendMessageW(GetDlgItem(hwnd_, kInvertY), BM_SETCHECK, s.aim.invertY ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(GetDlgItem(hwnd_, kCurve), BM_SETCHECK, s.aim.exponentialCurve ? BST_CHECKED : BST_UNCHECKED, 0);
}

void SettingsWindow::OnCommand(int id, int notification) {
    (void)notification;
    switch (id) {
        case kVeyraBrowse: {
            wchar_t file[MAX_PATH] = {};
            OPENFILENAMEW dialog{};
            dialog.lStructSize = sizeof(dialog);
            dialog.hwndOwner = hwnd_;
            dialog.lpstrFilter = L"Programas (*.exe)\0*.exe\0\0";
            dialog.lpstrFile = file;
            dialog.nMaxFile = MAX_PATH;
            dialog.lpstrTitle = L"Escolha o Veyra.exe";
            dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
            if (GetOpenFileNameW(&dialog)) ui::SetText(GetDlgItem(hwnd_, kVeyraPath), ui::ToUtf8(file));
            break;
        }
        case kVeyraDetect:
            SetMessage("Procurando o Veyra no computador...", ui::kMuted);
            if (const auto found = FindVeyraExecutable()) {
                ui::SetText(GetDlgItem(hwnd_, kVeyraPath), ui::ToUtf8(*found));
                SetMessage("Veyra encontrado.", ui::kGood);
            } else {
                SetMessage("Não achei o Veyra.exe. Escolha o arquivo com Procurar.", ui::kBad);
            }
            break;
        case kBindings:
            OpenBindings();
            break;
        case kAdvanced:
            ToggleAdvanced();
            break;
        case kForget:
            ForgetPairing();
            break;
        case kSetup:
            if (onSetup_) onSetup_();
            break;
        case kCancel:
            DestroyWindow(hwnd_);
            break;
        case kSave:
            if (Save()) DestroyWindow(hwnd_);
            break;
        default:
            break;
    }
}

void SettingsWindow::ToggleAdvanced() { ApplyLayout(); }

void SettingsWindow::ApplyLayout() {
    const bool show = SendMessageW(GetDlgItem(hwnd_, kAdvanced), BM_GETCHECK, 0, 0) == BST_CHECKED;
    for (HWND control : advanced_) ShowWindow(control, show ? SW_SHOW : SW_HIDE);

    const int bottomY = show ? kBottomExpandedY : kBottomCollapsedY;
    for (int id : {kForget, kSetup, kCancel, kSave}) MoveToY(hwnd_, id, bottomY);
    MoveToY(hwnd_, kMessage, bottomY + kMessageGap);

    RECT frame{0, 0, kWidth, show ? kHeightExpanded : kHeightCollapsed};
    AdjustWindowRect(&frame, kStyle, FALSE);
    SetWindowPos(hwnd_, nullptr, 0, 0, frame.right - frame.left, frame.bottom - frame.top,
                 SWP_NOMOVE | SWP_NOZORDER);
}

void SettingsWindow::OpenBindings() {
    if (bindings_ && !bindings_->IsOpen()) bindings_.reset();
    if (bindings_) {
        bindings_->Show();
        return;
    }
    bindings_ = std::make_unique<BindingsWindow>(instance_, app_);
}

bool SettingsWindow::Save() {
    AppSettings& s = app_.Settings();

    const LRESULT profile = SendMessageW(GetDlgItem(hwnd_, kProfile), CB_GETCURSEL, 0, 0);
    s.profileId = profile == kProfileDriving ? "driving" : "gta6";

    const std::string veyra = ui::GetText(GetDlgItem(hwnd_, kVeyraPath));
    if (!veyra.empty()) {
        std::error_code ec;
        if (!std::filesystem::is_regular_file(ui::ToWide(veyra), ec)) {
            SetMessage("Esse caminho do Veyra não existe. Deixe em branco para detectar sozinho.", ui::kBad);
            return false;
        }
    }
    s.veyraPath = veyra;

    const auto hotkey = InputCodeFromName(ui::GetText(GetDlgItem(hwnd_, kHotkey)));
    if (!hotkey) {
        SetMessage("Tecla desconhecida. Use uma tecla de função, como F8.", ui::kBad);
        return false;
    }
    s.captureHotkeyVk = *hotkey;

    const auto sensX = ParseFloat(ui::GetText(GetDlgItem(hwnd_, kSensX)), 0.1f, 5.0f);
    const auto sensY = ParseFloat(ui::GetText(GetDlgItem(hwnd_, kSensY)), 0.1f, 5.0f);
    const auto aimSensX = ParseFloat(ui::GetText(GetDlgItem(hwnd_, kAimSensX)), 0.1f, 5.0f);
    const auto aimSensY = ParseFloat(ui::GetText(GetDlgItem(hwnd_, kAimSensY)), 0.1f, 5.0f);
    const auto deadzone = ParseFloat(ui::GetText(GetDlgItem(hwnd_, kDeadzone)), 0.0f, 0.5f);
    const auto antiDeadzone = ParseFloat(ui::GetText(GetDlgItem(hwnd_, kAntiDeadzone)), 0.0f, 0.5f);
    const auto acceleration = ParseFloat(ui::GetText(GetDlgItem(hwnd_, kAcceleration)), 0.0f, 1.0f);
    const auto smoothing = ParseFloat(ui::GetText(GetDlgItem(hwnd_, kSmooth)), 0.0f, 200.0f);
    const auto curveExponent = ParseFloat(ui::GetText(GetDlgItem(hwnd_, kCurveExp)), 1.0f, 3.0f);
    if (!sensX || !sensY || !aimSensX || !aimSensY || !deadzone || !antiDeadzone || !acceleration || !smoothing ||
        !curveExponent) {
        SetMessage("Ajustes de mira fora do intervalo. Veja os limites ao lado de cada campo.", ui::kBad);
        return false;
    }
    s.aim.sensitivityX = *sensX;
    s.aim.sensitivityY = *sensY;
    s.aim.aimSensitivityX = *aimSensX;
    s.aim.aimSensitivityY = *aimSensY;
    s.aim.deadzone = *deadzone;
    s.aim.antiDeadzone = *antiDeadzone;
    s.aim.acceleration = *acceleration;
    s.aim.smoothingMs = *smoothing;
    s.aim.curveExponent = *curveExponent;
    s.aim.invertY = SendMessageW(GetDlgItem(hwnd_, kInvertY), BM_GETCHECK, 0, 0) == BST_CHECKED;
    s.aim.exponentialCurve = SendMessageW(GetDlgItem(hwnd_, kCurve), BM_GETCHECK, 0, 0) == BST_CHECKED;

    auto checked = [this](int id) { return SendMessageW(GetDlgItem(hwnd_, id), BM_GETCHECK, 0, 0) == BST_CHECKED; };
    s.autoConnectOnLaunch = checked(kAutoConnect);
    s.autoReconnect = checked(kAutoReconnect);
    s.minimizeToTray = checked(kMinimizeToTray);
    s.launchVeyra = checked(kLaunchVeyra);

    if (!app_.SaveSettings()) {
        SetMessage("Não consegui salvar as configurações.", ui::kBad);
        return false;
    }
    return true;
}

void SettingsWindow::ForgetPairing() {
    const int answer = MessageBoxW(hwnd_, L"Esquecer o PS5 pareado? Você vai precisar parear de novo.",
                                   L"PS5 Keyboard Bridge", MB_YESNO | MB_ICONQUESTION);
    if (answer != IDYES) return;
    if (DeleteSavedPairing()) {
        SetMessage("PS5 esquecido. Use a configuração inicial para parear de novo.", ui::kGood);
    } else {
        SetMessage("Nenhum PS5 pareado para esquecer.", ui::kMuted);
    }
}

void SettingsWindow::SetMessage(const char* text, COLORREF color) {
    HWND message = GetDlgItem(hwnd_, kMessage);
    ui::SetText(message, text);
    ui::SetColor(message, color);
}

}  // namespace pskb
