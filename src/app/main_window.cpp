#include "app/main_window.h"

#include <cwchar>
#include <utility>

#include "app/saved_pairing.h"
#include "app/ui.h"
#include "core/app_model.h"

namespace pskb {
namespace {

constexpr wchar_t kClassName[] = L"PS5KeyboardBridge.Main";
constexpr UINT kMsgTray = WM_APP + 1;
constexpr UINT_PTR kRefreshTimer = 1;
constexpr UINT kRefreshMs = 250;
constexpr int kWidth = 760;
constexpr int kHeight = 560;
constexpr DWORD kStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

enum Id : int {
    kTitleLabel = 300,
    kStatusLabel,
    kHintLabel,
    kPairLabel,
    kVeyraLabel,
    kCaptureLabel,
    kMessageLabel,
    kPlayButton,
    kStopButton,
    kSettingsButton,
    kSetupButton,
    kMenuOpen = 400,
    kMenuToggle,
    kMenuQuit,
};

COLORREF ColorFor(AppStatus status) {
    switch (status) {
        case AppStatus::Disconnected: return ui::kMuted;
        case AppStatus::Connecting: return ui::kWarn;
        case AppStatus::Connected: return ui::kAccent;
        case AppStatus::Playing: return ui::kGood;
        case AppStatus::Error: return ui::kBad;
    }
    return ui::kText;
}

}  // namespace

MainWindow::MainWindow(HINSTANCE instance, AppController& app) : instance_(instance), app_(app) {}

MainWindow::~MainWindow() {
    closing_ = true;
    if (hwnd_) DestroyWindow(hwnd_);
}

HWND MainWindow::FindRunningInstance() { return FindWindowW(kClassName, nullptr); }

bool MainWindow::Create() {
    static const bool registered = [this] {
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = &MainWindow::WndProc;
        wc.hInstance = instance_;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
        wc.hbrBackground = ui::WindowBackground();
        wc.lpszClassName = kClassName;
        return RegisterClassExW(&wc) != 0;
    }();
    if (!registered) return false;

    RECT frame{0, 0, kWidth, kHeight};
    AdjustWindowRect(&frame, kStyle, FALSE);
    hwnd_ = CreateWindowExW(WS_EX_CONTROLPARENT, kClassName, L"PS5 Keyboard Bridge", kStyle, CW_USEDEFAULT,
                            CW_USEDEFAULT, frame.right - frame.left, frame.bottom - frame.top, nullptr, nullptr,
                            instance_, this);
    if (!hwnd_) return false;

    Layout();
    RefreshPairing();
    Refresh();
    SetTimer(hwnd_, kRefreshTimer, kRefreshMs, nullptr);
    return true;
}

void MainWindow::Show() {
    ShowWindow(hwnd_, SW_SHOW);
    SetForegroundWindow(hwnd_);
}

void MainWindow::Layout() {
    ui::Label(hwnd_, "PS5 Keyboard Bridge", kTitleLabel, 40, 26, 680, 36, ui::Font(28, FW_BOLD));
    statusLabel_ = ui::Label(hwnd_, "", kStatusLabel, 40, 74, 680, 64, ui::Font(50, FW_BOLD), ui::kMuted);
    hintLabel_ = ui::Label(hwnd_, "", kHintLabel, 40, 144, 680, 28, ui::Font(18), ui::kMuted);
    pairLabel_ = ui::Label(hwnd_, "", kPairLabel, 40, 192, 680, 24, ui::Font(17));
    veyraLabel_ = ui::Label(hwnd_, "", kVeyraLabel, 40, 220, 680, 24, ui::Font(17));
    captureLabel_ = ui::Label(hwnd_, "", kCaptureLabel, 40, 248, 680, 24, ui::Font(17));
    playButton_ = ui::Button(hwnd_, "JOGAR", kPlayButton, 40, 292, 680, 112, true);
    stopButton_ = ui::Button(hwnd_, "PARAR", kStopButton, 40, 424, 170, 56);
    ui::Button(hwnd_, "Configurações", kSettingsButton, 230, 424, 250, 56);
    ui::Button(hwnd_, "Configuração inicial", kSetupButton, 500, 424, 220, 56);
    messageLabel_ = ui::Label(hwnd_, "", kMessageLabel, 40, 500, 680, 28, ui::Font(17), ui::kBad);
}

void MainWindow::RefreshPairing() {
    const auto pairing = LoadSavedPairing();
    paired_ = pairing.has_value();
    pairedName_ = pairing ? pairing->name : std::string{};
}

void MainWindow::Refresh() {
    if (closing_ || !hwnd_) return;
    app_.Tick();

    const AppStatus status = app_.Status();
    ui::SetText(statusLabel_, AppStatusText(status));
    ui::SetColor(statusLabel_, ColorFor(status));

    std::string hint;
    COLORREF hintColor = ui::kMuted;
    switch (status) {
        case AppStatus::Disconnected:
            hint = paired_ ? "Pronto para jogar. Pressione JOGAR." : "Faça a configuração inicial para parear seu PS5.";
            break;
        case AppStatus::Connecting:
            hint = "Conectando ao PS5...";
            break;
        case AppStatus::Connected:
            hint = "Pronto para jogar. Abra o Veyra e pressione F8 para capturar.";
            hintColor = ui::kGood;
            break;
        case AppStatus::Playing:
            hint = "Jogando. F8 liga e desliga a captura; Esc libera tudo.";
            hintColor = ui::kGood;
            break;
        case AppStatus::Error:
            hint = "A conexão parou. Pressione JOGAR para tentar de novo.";
            hintColor = ui::kBad;
            break;
    }
    ui::SetText(hintLabel_, hint);
    ui::SetColor(hintLabel_, hintColor);

    const bool playing = app_.IsPlaying();
    EnableWindow(playButton_, !playing);
    EnableWindow(stopButton_, playing);

    ui::SetText(pairLabel_, paired_ ? "PS5 pareado: " + pairedName_ : "PS5: ainda não pareado");

    const std::string& veyra = app_.Settings().veyraPath;
    ui::SetText(veyraLabel_, veyra.empty() ? "Veyra: não configurado. Use Configurações > Detectar." : "Veyra: " + veyra);

    switch (app_.Capture()) {
        case CaptureMode::Off:
            ui::SetText(captureLabel_, "Captura: desligada");
            break;
        case CaptureMode::Armed:
            ui::SetText(captureLabel_, "Captura: aguardando o Veyra em foco");
            break;
        case CaptureMode::Active:
            ui::SetText(captureLabel_, app_.CaptureBlocking()
                                           ? "Captura: ativa. Teclas e clique ficam bloqueados para o Veyra."
                                           : "Captura: ativa (AVISO: não consegui bloquear as teclas do Veyra)");
            break;
    }

    const std::string linkMessage = status == AppStatus::Error ? app_.StatusMessage() : std::string{};
    ui::SetText(messageLabel_, !lastError_.empty() ? lastError_ : linkMessage);
}

void MainWindow::ReapClosedWindows() {
    if (wizard_ && !wizard_->IsOpen()) wizard_.reset();
    if (settings_ && !settings_->IsOpen()) settings_.reset();
}

void MainWindow::OpenSetup() {
    if (wizard_) {
        wizard_->Show();
        return;
    }
    wizard_ = std::make_unique<SetupWizard>(instance_, app_, [this] {
        RefreshPairing();
        Refresh();
    });
}

void MainWindow::OpenSettings() {
    if (settings_) {
        settings_->Show();
        return;
    }
    settings_ = std::make_unique<SettingsWindow>(
        instance_, app_,
        [this] {
            RefreshPairing();
            Refresh();
        },
        [this] { OpenSetup(); });
}

void MainWindow::Play() {
    if (!paired_) {
        OpenSetup();
        return;
    }
    std::string error;
    lastError_ = app_.StartPlay(&error) ? std::string{} : error;
    Refresh();
}

void MainWindow::Stop() {
    app_.StopPlay();
    lastError_.clear();
    Refresh();
}

void MainWindow::OnCommand(int id) {
    switch (id) {
        case kPlayButton:
        case kMenuToggle:
            if (app_.IsPlaying()) {
                Stop();
            } else {
                Play();
            }
            break;
        case kStopButton:
            Stop();
            break;
        case kSettingsButton:
            OpenSettings();
            break;
        case kSetupButton:
            OpenSetup();
            break;
        case kMenuOpen:
            RestoreFromTray();
            break;
        case kMenuQuit:
            Quit();
            break;
        default:
            break;
    }
}

void MainWindow::HideToTray() {
    AddTrayIcon();
    ShowWindow(hwnd_, SW_HIDE);
}

void MainWindow::RestoreFromTray() {
    RemoveTrayIcon();
    ShowWindow(hwnd_, SW_SHOW);
    if (IsIconic(hwnd_)) ShowWindow(hwnd_, SW_RESTORE);
    SetForegroundWindow(hwnd_);
}

void MainWindow::AddTrayIcon() {
    if (trayVisible_) return;
    tray_ = {};
    tray_.cbSize = sizeof(tray_);
    tray_.hWnd = hwnd_;
    tray_.uID = 1;
    tray_.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    tray_.uCallbackMessage = kMsgTray;
    tray_.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wcscpy_s(tray_.szTip, L"PS5 Keyboard Bridge");
    trayVisible_ = Shell_NotifyIconW(NIM_ADD, &tray_) != FALSE;
}

void MainWindow::RemoveTrayIcon() {
    if (!trayVisible_) return;
    Shell_NotifyIconW(NIM_DELETE, &tray_);
    trayVisible_ = false;
}

void MainWindow::ShowTrayMenu() {
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, kMenuOpen, L"Abrir");
    AppendMenuW(menu, MF_STRING, kMenuToggle, app_.IsPlaying() ? L"Parar" : L"Jogar");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kMenuQuit, L"Sair");

    POINT cursor{};
    GetCursorPos(&cursor);
    SetForegroundWindow(hwnd_);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, cursor.x, cursor.y, 0, hwnd_, nullptr);
    DestroyMenu(menu);
}

void MainWindow::Quit() {
    quitting_ = true;
    DestroyWindow(hwnd_);
}

LRESULT CALLBACK MainWindow::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
    }
    auto* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self) return self->OnMessage(hwnd, message, wParam, lParam);
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT MainWindow::OnMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_COMMAND:
            OnCommand(LOWORD(wParam));
            return 0;
        case WM_TIMER:
            if (wParam == kRefreshTimer) {
                ReapClosedWindows();
                Refresh();
            }
            return 0;
        case WM_SIZE:
            if (wParam == SIZE_MINIMIZED && app_.Settings().minimizeToTray) HideToTray();
            return 0;
        case WM_CLOSE:
            if (!quitting_ && app_.Settings().minimizeToTray) {
                HideToTray();
                return 0;
            }
            DestroyWindow(hwnd);
            return 0;
        case kMsgTray:
            switch (LOWORD(lParam)) {
                case WM_LBUTTONUP:
                case WM_LBUTTONDBLCLK:
                    RestoreFromTray();
                    break;
                case WM_RBUTTONUP:
                    ShowTrayMenu();
                    break;
                default:
                    break;
            }
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
        case WM_DESTROY:
            closing_ = true;
            KillTimer(hwnd, kRefreshTimer);
            app_.StopPlay();
            RemoveTrayIcon();
            wizard_.reset();
            settings_.reset();
            hwnd_ = nullptr;
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

}  // namespace pskb
