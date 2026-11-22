#include "app/setup_wizard.h"

#include <commdlg.h>

#include <chrono>
#include <cstdio>
#include <cwchar>
#include <iterator>
#include <utility>

#include "app/saved_pairing.h"
#include "app/ui.h"
#include "backend/chiaki_backend.h"
#include "backend/psn_signin.h"
#include "core/psn_login.h"
#include "core/secure_store.h"
#include "core/windows_support.h"
#include "platform/veyra_locator.h"
#include "platform/windows_version.h"

namespace pskb {
namespace {

constexpr wchar_t kClassName[] = L"PS5KeyboardBridge.Wizard";
constexpr UINT kMsgTaskDone = WM_APP + 20;
constexpr int kWidth = 760;
constexpr int kHeight = 570;
constexpr DWORD kStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

enum Id : int {
    kStep = 100,
    kTitle,
    kStatus,
    kBack,
    kNext,
    kPageLabel = 200,
    kProbe,
    kConsoleList,
    kManualIp,
    kManualUse,
    kSignInOpen,
    kRedirect,
    kSignInComplete,
    kAccountText,
    kAccountUse,
    kPin,
    kPair,
    kTest,
    kVeyraPath,
    kVeyraFind,
    kVeyraBrowse,
    kLaunchVeyra,
};

bool IsControlClass(HWND control, const wchar_t* name) {
    wchar_t buffer[32] = {};
    GetClassNameW(control, buffer, static_cast<int>(std::size(buffer)));
    return _wcsicmp(buffer, name) == 0;
}

}  // namespace

SetupWizard::SetupWizard(HINSTANCE instance, AppController& app, std::function<void()> onClosed)
    : app_(app), onClosed_(std::move(onClosed)) {
    const AppSettings& settings = app_.Settings();
    launchVeyra_ = settings.launchVeyra;
    if (!settings.veyraPath.empty()) veyraPath_ = ui::ToWide(settings.veyraPath);
    Create(instance);
    ShowStep(WizardStep::Welcome);
    Show();
}

SetupWizard::~SetupWizard() {
    if (hwnd_) DestroyWindow(hwnd_);
    if (worker_.joinable()) worker_.join();
}

void SetupWizard::Show() {
    ShowWindow(hwnd_, SW_SHOW);
    SetForegroundWindow(hwnd_);
}

void SetupWizard::Create(HINSTANCE instance) {
    static const bool registered = [instance] {
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = &SetupWizard::WndProc;
        wc.hInstance = instance;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = ui::WindowBackground();
        wc.lpszClassName = kClassName;
        return RegisterClassExW(&wc) != 0;
    }();
    if (!registered) return;

    RECT frame{0, 0, kWidth, kHeight};
    AdjustWindowRect(&frame, kStyle, FALSE);
    hwnd_ = CreateWindowExW(WS_EX_CONTROLPARENT, kClassName, L"Configuração inicial - PS5 Keyboard Bridge", kStyle,
                            CW_USEDEFAULT, CW_USEDEFAULT, frame.right - frame.left, frame.bottom - frame.top,
                            nullptr, nullptr, instance, this);

    ui::Label(hwnd_, "", kStep, 40, 22, 680, 22, ui::Font(16), ui::kMuted);
    ui::Label(hwnd_, "", kTitle, 40, 46, 680, 44, ui::Font(32, FW_BOLD));
    ui::Label(hwnd_, "", kStatus, 40, 452, 680, 40, ui::Font(17), ui::kMuted);
    ui::Button(hwnd_, "Voltar", kBack, 40, 504, 160, 44);
    ui::Button(hwnd_, "Avançar", kNext, 560, 504, 160, 44, true);
}

LRESULT CALLBACK SetupWizard::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
    }
    auto* self = reinterpret_cast<SetupWizard*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self) return self->OnMessage(hwnd, message, wParam, lParam);
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT SetupWizard::OnMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_COMMAND:
            OnCommand(LOWORD(wParam), HIWORD(wParam));
            return 0;
        case kMsgTaskDone:
            OnTaskDone();
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
            if (busy_) {
                SetStatus("Aguarde a tarefa terminar antes de fechar.", ui::kMuted);
                return 0;
            }
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            hwnd_ = nullptr;
            if (worker_.joinable()) worker_.join();
            if (onClosed_) onClosed_();
            return 0;
        default:
            break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

void SetupWizard::ShowStep(WizardStep step) {
    step_ = step;
    ClearPage();
    ui::SetText(GetDlgItem(hwnd_, kStep),
                "Passo " + std::to_string(WizardStepNumber(step)) + " de " + std::to_string(kWizardStepCount));
    ui::SetText(GetDlgItem(hwnd_, kTitle), WizardStepTitle(step));
    SetStatus("", ui::kMuted);
    BuildPage();
    UpdateNavigation();
}

void SetupWizard::ClearPage() {
    for (HWND control : page_) DestroyWindow(control);
    page_.clear();
}

HWND SetupWizard::Add(HWND control) {
    page_.push_back(control);
    return control;
}

void SetupWizard::BuildPage() {
    switch (step_) {
        case WizardStep::Welcome: BuildWelcome(); break;
        case WizardStep::Console: BuildConsole(); break;
        case WizardStep::RemotePlay: BuildRemotePlay(); break;
        case WizardStep::Account: BuildAccount(); break;
        case WizardStep::Pin: BuildPin(); break;
        case WizardStep::Test: BuildTest(); break;
        case WizardStep::Veyra: BuildVeyra(); break;
        case WizardStep::Done: BuildDone(); break;
    }
}

void SetupWizard::BuildWelcome() {
    Add(ui::Label(hwnd_,
                  "Este assistente conecta seu PS5 ao PC para você jogar com teclado e mouse. O PS5 continua "
                  "mandando a imagem para a sua capturadora normalmente; este app só envia os comandos do controle.",
                  kPageLabel, 40, 120, 680, 80, ui::Font(19)));
    Add(ui::Label(hwnd_,
                  "Você vai precisar do PS5 ligado (ou em repouso com \"Ligar pela rede\"), do PC e do PS5 na mesma "
                  "rede, e da conta PSN que você usa no PS5. Leva uns cinco minutos.",
                  kPageLabel, 40, 220, 680, 80, ui::Font(19)));
    const WindowsVersion windows = CurrentWindowsVersion();
    const WindowsSupport support = CheckWindowsSupport(windows);
    switch (support) {
        case WindowsSupport::Supported:
            Add(ui::Label(hwnd_, "Windows compatível.", kPageLabel, 40, 330, 680, 30, ui::Font(20, FW_SEMIBOLD),
                          ui::kGood));
            break;
        case WindowsSupport::Untested:
            Add(ui::Label(hwnd_, "Windows antigo: pode funcionar, mas não foi testado.", kPageLabel, 40, 330, 680, 30,
                          ui::Font(20, FW_SEMIBOLD), ui::kWarn));
            Add(ui::Label(hwnd_, WindowsSupportMessage(windows, support), kPageLabel, 40, 366, 680, 50, ui::Font(17),
                          ui::kMuted));
            break;
        case WindowsSupport::Unsupported:
            Add(ui::Label(hwnd_, "Este Windows não é compatível.", kPageLabel, 40, 330, 680, 30,
                          ui::Font(20, FW_SEMIBOLD), ui::kBad));
            Add(ui::Label(hwnd_, WindowsSupportMessage(windows, support), kPageLabel, 40, 366, 680, 50, ui::Font(17),
                          ui::kMuted));
            break;
    }
}

void SetupWizard::BuildConsole() {
    Add(ui::Label(hwnd_, "Deixe o PS5 ligado ou em repouso e clique em Procurar PS5.", kPageLabel, 40, 116, 680, 26,
                  ui::Font(19)));
    Add(ui::Button(hwnd_, "Procurar PS5", kProbe, 40, 150, 240, 44));
    Add(ui::ListBox(hwnd_, kConsoleList, 40, 206, 680, 130));
    Add(ui::Label(hwnd_, "Não apareceu? Digite o IP do PS5:", kPageLabel, 40, 350, 680, 26, ui::Font(18)));
    Add(ui::Edit(hwnd_, kManualIp, 40, 380, 300, 30, 64));
    Add(ui::Button(hwnd_, "Usar este IP", kManualUse, 352, 378, 160, 36));
    FillConsoleList();
}

void SetupWizard::FillConsoleList() {
    HWND list = GetDlgItem(hwnd_, kConsoleList);
    if (!list) return;
    SendMessageW(list, LB_RESETCONTENT, 0, 0);
    int selected = -1;
    for (std::size_t i = 0; i < consoles_.size(); ++i) {
        const ConsoleInfo& c = consoles_[i];
        const std::string text = c.name + "   " + c.host + "   " + (c.ready ? "pronto" : "em repouso");
        SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(ui::ToWide(text).c_str()));
        if (chosen_ && chosen_->host == c.host) selected = static_cast<int>(i);
    }
    if (selected >= 0) SendMessageW(list, LB_SETCURSEL, static_cast<WPARAM>(selected), 0);
}

void SetupWizard::BuildRemotePlay() {
    Add(ui::Label(hwnd_,
                  "No PS5, ative o Remote Play:\r\n\r\n"
                  "1. Configurações > Sistema > Remote Play > marque \"Ativar Remote Play\".\r\n\r\n"
                  "2. Configurações > Sistema > Economia de energia > Recursos disponíveis em modo de repouso: "
                  "marque \"Manter conectado à Internet\" e \"Ativar ligação do PS5 pela rede\".\r\n\r\n"
                  "3. Deixe o PS5 e o PC na mesma rede.\r\n\r\n"
                  "Depois de conferir, clique em Avançar.",
                  kPageLabel, 40, 120, 680, 300, ui::Font(19)));
}

void SetupWizard::BuildAccount() {
    Add(ui::Label(hwnd_,
                  "Para parear, o app precisa do ID da sua conta PSN. Você entra no site oficial da Sony no navegador; "
                  "a senha nunca passa por este app.",
                  kPageLabel, 40, 116, 680, 50, ui::Font(18)));
    Add(ui::Button(hwnd_, "1. Abrir login da Sony", kSignInOpen, 40, 172, 280, 44));
    Add(ui::Label(hwnd_, "2. Depois do login, copie a URL inteira da página \"redirect\" e cole aqui:", kPageLabel,
                  40, 232, 680, 26, ui::Font(18)));
    Add(ui::Edit(hwnd_, kRedirect, 40, 262, 680, 30, 2048));
    Add(ui::Button(hwnd_, "3. Concluir login", kSignInComplete, 40, 306, 280, 44));
    Add(ui::Label(hwnd_, "Se você já sabe o ID da conta, digite aqui:", kPageLabel, 40, 364, 680, 26, ui::Font(18)));
    Add(ui::Edit(hwnd_, kAccountText, 40, 394, 300, 30, 32));
    Add(ui::Button(hwnd_, "Usar este ID", kAccountUse, 352, 392, 160, 36));
    if (account_) {
        SetStatus("Conta reconhecida: " + psn_login::AccountIdToBase64(*account_), ui::kGood);
    }
}

void SetupWizard::BuildPin() {
    Add(ui::Label(hwnd_,
                  "No PS5, abra Configurações > Sistema > Remote Play > Link Device. A tela mostra um código de 8 "
                  "dígitos que vale por alguns minutos. Digite o código aqui.",
                  kPageLabel, 40, 116, 680, 80, ui::Font(18)));
    Add(ui::Edit(hwnd_, kPin, 40, 220, 260, 36, 12));
    Add(ui::Button(hwnd_, "Parear com o PS5", kPair, 320, 216, 260, 44, true));
    if (progress_.paired) SetStatus("Pareado. Pode seguir.", ui::kGood);
}

void SetupWizard::BuildTest() {
    Add(ui::Label(hwnd_,
                  "Vou conectar ao PS5 e enviar um toque no botão X, para confirmar que os comandos chegam. "
                  "A imagem não é usada nesse teste.",
                  kPageLabel, 40, 116, 680, 80, ui::Font(19)));
    Add(ui::Button(hwnd_, "Conectar e testar", kTest, 40, 210, 280, 48, true));
    if (progress_.connectionTested) SetStatus("Teste concluído com sucesso.", ui::kGood);
}

void SetupWizard::BuildVeyra() {
    Add(ui::Label(hwnd_,
                  "O app abre o Veyra quando você joga, sem mudar nenhuma configuração dele. Se não achar sozinho, "
                  "escolha o Veyra.exe.",
                  kPageLabel, 40, 116, 680, 60, ui::Font(19)));
    const std::string path = veyraPath_ ? "Veyra: " + ui::ToUtf8(*veyraPath_) : "Veyra: ainda não encontrado.";
    Add(ui::Label(hwnd_, path, kVeyraPath, 40, 186, 680, 30, ui::Font(18)));
    Add(ui::Button(hwnd_, "Procurar automaticamente", kVeyraFind, 40, 230, 280, 44));
    Add(ui::Button(hwnd_, "Escolher Veyra.exe...", kVeyraBrowse, 340, 230, 280, 44));
    Add(ui::Check(hwnd_, "Abrir o Veyra automaticamente ao jogar", kLaunchVeyra, 40, 300, 680, 28, launchVeyra_));
}

void SetupWizard::BuildDone() {
    Add(ui::Label(hwnd_, "Configuração concluída.", kPageLabel, 40, 130, 680, 60, ui::Font(42, FW_BOLD), ui::kGood));
    Add(ui::Label(hwnd_,
                  "Abra o Veyra e depois pressione JOGAR na tela principal. Quando o Veyra estiver em foco, seu teclado "
                  "e mouse controlam o jogo no PS5. F8 liga e desliga a captura; Esc libera tudo.\r\n\r\n"
                  "O PS5 continua mostrando a imagem na sua capturadora.",
                  kPageLabel, 40, 210, 680, 170, ui::Font(19)));
}

void SetupWizard::UpdateNavigation() {
    for (HWND control : page_) {
        if (IsControlClass(control, L"BUTTON") || IsControlClass(control, L"EDIT") ||
            IsControlClass(control, L"LISTBOX")) {
            EnableWindow(control, !busy_);
        }
    }
    EnableWindow(GetDlgItem(hwnd_, kBack), !busy_ && step_ != WizardStep::Welcome);
    EnableWindow(GetDlgItem(hwnd_, kNext), !busy_ && CanAdvance(step_, progress_));
    ui::SetText(GetDlgItem(hwnd_, kNext), step_ == WizardStep::Done ? "Concluir" : "Avançar");
}

void SetupWizard::SetStatus(const std::string& text, COLORREF color) {
    HWND status = GetDlgItem(hwnd_, kStatus);
    ui::SetText(status, text);
    ui::SetColor(status, color);
}

void SetupWizard::OnCommand(int id, int notification) {
    switch (id) {
        case kBack:
            Back();
            break;
        case kNext:
            Next();
            break;

        case kProbe:
            SetStatus("Procurando o PS5 na rede. Pode levar alguns segundos...", ui::kMuted);
            StartTask(Task::Probe, [](TaskResults& r) {
                for (const ConsoleInfo& c : ProbeConsoles("", 4000, nullptr)) {
                    if (c.isPs5) r.consoles.push_back(c);
                }
                r.ok = !r.consoles.empty();
                r.message = r.ok ? "" : "Nenhum PS5 respondeu. Ligue o PS5 ou deixe-o em repouso com \"Ligar pela rede\".";
            });
            break;

        case kConsoleList: {
            if (notification != LBN_SELCHANGE) break;
            const int selected = static_cast<int>(SendMessageW(GetDlgItem(hwnd_, kConsoleList), LB_GETCURSEL, 0, 0));
            if (selected < 0 || static_cast<std::size_t>(selected) >= consoles_.size()) break;
            chosen_ = consoles_[static_cast<std::size_t>(selected)];
            progress_.consoleChosen = true;
            SetStatus("PS5 escolhido: " + chosen_->name, ui::kGood);
            UpdateNavigation();
            break;
        }

        case kManualUse: {
            const std::string host = ui::GetText(GetDlgItem(hwnd_, kManualIp));
            if (host.empty()) {
                SetStatus("Digite o IP do PS5.", ui::kBad);
                break;
            }
            SetStatus("Procurando o PS5 nesse endereço...", ui::kMuted);
            StartTask(Task::ManualProbe, [host](TaskResults& r) {
                for (const ConsoleInfo& c : ProbeConsoles(host, 3000, nullptr)) {
                    if (c.isPs5) {
                        r.console = c;
                        break;
                    }
                }
                r.ok = r.console.has_value();
                r.message = r.ok ? "" : "Não encontrei um PS5 nesse IP.";
            });
            break;
        }

        case kSignInOpen:
            if (psn_signin::OpenSignInPage()) {
                SetStatus("Login aberto no navegador. Depois do login, cole a URL redirect aqui.", ui::kMuted);
            } else {
                SetStatus("Não consegui abrir o navegador. Abra este endereço: " + psn_login::SignInUrl(), ui::kBad);
            }
            break;

        case kSignInComplete: {
            HWND redirect = GetDlgItem(hwnd_, kRedirect);
            std::string url = ui::GetText(redirect);
            SetWindowTextW(redirect, L"");
            if (url.empty()) {
                SetStatus("Cole a URL da página redirect primeiro.", ui::kBad);
                break;
            }
            SetStatus("Confirmando a conta com a Sony...", ui::kMuted);
            StartTask(Task::SignIn, [url = std::move(url)](TaskResults& r) mutable {
                const auto result = psn_signin::CompleteSignIn(url);
                SecureZeroMemory(url.data(), url.size());
                r.ok = result.ok;
                r.message = result.message;
                if (result.ok) r.account = result.account;
            });
            break;
        }

        case kAccountUse: {
            const std::string text = ui::GetText(GetDlgItem(hwnd_, kAccountText));
            const auto account = ParseAccountId(text);
            if (!account) {
                SetStatus("ID inválido. Use o número ou o código de 12 caracteres.", ui::kBad);
                break;
            }
            account_ = account;
            progress_.accountReady = true;
            SetStatus("Conta reconhecida: " + psn_login::AccountIdToBase64(*account), ui::kGood);
            UpdateNavigation();
            break;
        }

        case kPair: {
            const auto pin = NormalizePin(ui::GetText(GetDlgItem(hwnd_, kPin)));
            if (!pin) {
                SetStatus("O código tem 8 dígitos. Exemplo: 1234 5678.", ui::kBad);
                break;
            }
            if (!chosen_ || !account_) {
                SetStatus("Escolha o PS5 e conclua a conta antes de parear.", ui::kBad);
                break;
            }
            SetStatus("Pareando com o PS5. Pode levar alguns segundos...", ui::kMuted);
            StartTask(Task::Pair, [host = chosen_->host, account = *account_, code = *pin](TaskResults& r) {
                const auto result = chiaki_backend::RegisterConsole(host, account, code);
                r.ok = result.ok;
                r.message = result.message;
                if (!result.ok) return;
                if (SaveSavedPairing(result.pairing)) {
                    r.message = "Pareado com " + result.pairing.name + ".";
                } else {
                    r.ok = false;
                    r.message = "Não foi possível salvar o pareamento com a proteção do Windows (DPAPI).";
                }
            });
            break;
        }

        case kTest:
            SetStatus("Conectando ao PS5...", ui::kMuted);
            StartTask(Task::Test, [](TaskResults& r) {
                const auto pairing = LoadSavedPairing();
                if (!pairing) {
                    r.message = "Nenhum pareamento salvo. Volte e pareie o PS5.";
                    return;
                }
                ChiakiBackend backend;
                const ConnectResult result = backend.Connect(*pairing);
                r.ok = result.ok;
                r.message = result.message;
                if (result.ok) {
                    PadState pressed;
                    pressed.buttons = PadCross;
                    for (int i = 0; i < 6; ++i) {
                        backend.SendPad(pressed);
                        std::this_thread::sleep_for(std::chrono::milliseconds(80));
                    }
                    backend.SendPad(PadState{});
                    r.message = "Conectado. Enviei um toque no botão X ao PS5.";
                }
                backend.Disconnect();
            });
            break;

        case kVeyraFind:
            SetStatus("Procurando o Veyra no computador...", ui::kMuted);
            StartTask(Task::FindVeyra, [](TaskResults& r) {
                r.veyra = FindVeyraExecutable();
                r.ok = r.veyra.has_value();
                r.message = r.ok ? "" : "Não achei o Veyra.exe. Escolha o arquivo manualmente.";
            });
            break;

        case kVeyraBrowse: {
            wchar_t file[MAX_PATH] = {};
            OPENFILENAMEW dialog{};
            dialog.lStructSize = sizeof(dialog);
            dialog.hwndOwner = hwnd_;
            dialog.lpstrFilter = L"Veyra.exe\0Veyra.exe\0Programas (*.exe)\0*.exe\0\0";
            dialog.lpstrFile = file;
            dialog.nMaxFile = MAX_PATH;
            dialog.lpstrTitle = L"Escolha o Veyra.exe";
            dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
            if (GetOpenFileNameW(&dialog)) {
                veyraPath_ = std::wstring(file);
                ui::SetText(GetDlgItem(hwnd_, kVeyraPath), "Veyra: " + ui::ToUtf8(*veyraPath_));
                SetStatus("Veyra escolhido.", ui::kGood);
            }
            break;
        }

        case kLaunchVeyra:
            launchVeyra_ = SendMessageW(GetDlgItem(hwnd_, kLaunchVeyra), BM_GETCHECK, 0, 0) == BST_CHECKED;
            break;

        default:
            break;
    }
}

void SetupWizard::Next() {
    if (busy_ || !CanAdvance(step_, progress_)) return;
    if (step_ == WizardStep::Done) {
        Finish();
        return;
    }
    ShowStep(NextWizardStep(step_));
}

void SetupWizard::Back() {
    if (busy_) return;
    ShowStep(PreviousWizardStep(step_));
}

void SetupWizard::Finish() {
    AppSettings& settings = app_.Settings();
    settings.launchVeyra = launchVeyra_;
    if (veyraPath_) settings.veyraPath = ui::ToUtf8(*veyraPath_);
    app_.SaveSettings();
    DestroyWindow(hwnd_);
}

void SetupWizard::StartTask(Task task, std::function<void(TaskResults&)> work) {
    if (busy_) return;
    busy_ = true;
    running_ = task;
    UpdateNavigation();

    HWND target = hwnd_;
    worker_ = std::thread([this, target, work = std::move(work)] {
        TaskResults results;
        work(results);
        {
            std::lock_guard<std::mutex> lock(resultMutex_);
            results_ = std::move(results);
        }
        PostMessageW(target, kMsgTaskDone, 0, 0);
    });
}

void SetupWizard::OnTaskDone() {
    if (worker_.joinable()) worker_.join();
    busy_ = false;

    TaskResults r;
    {
        std::lock_guard<std::mutex> lock(resultMutex_);
        r = std::move(results_);
        results_ = {};
    }

    switch (running_) {
        case Task::Probe:
            consoles_ = r.consoles;
            FillConsoleList();
            if (r.ok) {
                SetStatus("Encontrei " + std::to_string(consoles_.size()) + " PS5. Clique no que for o seu.",
                          ui::kGood);
            } else {
                SetStatus(r.message, ui::kBad);
            }
            break;

        case Task::ManualProbe:
            if (r.ok && r.console) {
                chosen_ = r.console;
                progress_.consoleChosen = true;
                consoles_ = {*r.console};
                FillConsoleList();
                SetStatus("PS5 encontrado: " + chosen_->name, ui::kGood);
            } else {
                SetStatus(r.message, ui::kBad);
            }
            break;

        case Task::SignIn:
            if (r.ok && r.account) {
                account_ = r.account;
                progress_.accountReady = true;
                SetStatus("Conta reconhecida: " + psn_login::AccountIdToBase64(*account_), ui::kGood);
            } else {
                SetStatus(r.message, ui::kBad);
            }
            break;

        case Task::Pair:
            progress_.paired = r.ok;
            SetStatus(r.message, r.ok ? ui::kGood : ui::kBad);
            break;

        case Task::Test:
            progress_.connectionTested = r.ok;
            SetStatus(r.message, r.ok ? ui::kGood : ui::kBad);
            break;

        case Task::FindVeyra:
            if (r.veyra) {
                veyraPath_ = r.veyra;
                ui::SetText(GetDlgItem(hwnd_, kVeyraPath), "Veyra: " + ui::ToUtf8(*veyraPath_));
                SetStatus("Veyra encontrado.", ui::kGood);
            } else {
                SetStatus(r.message, ui::kBad);
            }
            break;
    }
    UpdateNavigation();
}

}  // namespace pskb
