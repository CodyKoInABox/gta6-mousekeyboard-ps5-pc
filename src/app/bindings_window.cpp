#include "app/bindings_window.h"

#include <algorithm>

#include "app/ui.h"

namespace pskb {
namespace {

constexpr wchar_t kClassName[] = L"PS5KeyboardBridge.Bindings";
constexpr int kWidth = 760;
constexpr int kHeight = 700;
constexpr DWORD kStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

enum Id : int {
    kTitle = 500,
    kProfileLabel,
    kProfile,
    kHint,
    kList,
    kChange,
    kRemove,
    kReset,
    kAction,
    kAdd,
    kMessage,
    kClose,
};

constexpr int kProfileGta = 0;
constexpr int kProfileDriving = 1;

// Key or mouse button pressed during capture. Shift, Ctrl, and Alt report generic codes in
// WM_KEYDOWN, so the scan code decides between the left and right key.
InputCode CapturedCode(WPARAM wParam, LPARAM lParam) {
    UINT vk = static_cast<UINT>(wParam);
    if (vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU) {
        const UINT bits = static_cast<UINT>(lParam);
        const UINT scan = (bits >> 16) & 0xFFu;
        const bool extended = ((bits >> 24) & 1u) != 0;
        const UINT mapped = MapVirtualKeyW(scan | (extended ? 0xE000u : 0u), MAPVK_VSC_TO_VK_EX);
        if (mapped != 0) vk = mapped;
    }
    return static_cast<InputCode>(vk);
}

}  // namespace

BindingsWindow::BindingsWindow(HINSTANCE instance, AppController& app) : app_(app) {
    Create(instance);
    if (!hwnd_) return;
    LoadProfile(app_.Settings().profileId);
    Show();
}

BindingsWindow::~BindingsWindow() {
    EndCapture();
    if (hwnd_) DestroyWindow(hwnd_);
}

void BindingsWindow::Show() {
    ShowWindow(hwnd_, SW_SHOW);
    SetForegroundWindow(hwnd_);
}

void BindingsWindow::Create(HINSTANCE instance) {
    static const bool registered = [instance] {
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = &BindingsWindow::WndProc;
        wc.hInstance = instance;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = ui::WindowBackground();
        wc.lpszClassName = kClassName;
        return RegisterClassExW(&wc) != 0;
    }();
    if (!registered) return;

    RECT frame{0, 0, kWidth, kHeight};
    AdjustWindowRect(&frame, kStyle, FALSE);
    hwnd_ = CreateWindowExW(WS_EX_CONTROLPARENT, kClassName, L"Teclas do perfil - PS5 Keyboard Bridge", kStyle,
                            CW_USEDEFAULT, CW_USEDEFAULT, frame.right - frame.left, frame.bottom - frame.top,
                            nullptr, nullptr, instance, this);

    ui::Label(hwnd_, "Teclas do perfil", kTitle, 40, 24, 680, 36, ui::Font(26, FW_BOLD));
    ui::Label(hwnd_, "Perfil", kProfileLabel, 40, 84, 200, 24, ui::Font(18));
    profileCombo_ = ui::ComboBox(hwnd_, kProfile, 260, 80, 440, 200);
    SendMessageW(profileCombo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"GTA 6 (padrão)"));
    SendMessageW(profileCombo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Dirigir (volante suave)"));

    ui::Label(hwnd_,
              "Escolha uma linha e clique em Alterar tecla, ou escolha uma ação e clique em Adicionar tecla. "
              "Depois pressione a tecla ou clique com o mouse. Mudanças valem na próxima vez que você pressionar "
              "JOGAR.",
              kHint, 40, 118, 680, 76, ui::Font(16), ui::kMuted);

    list_ = ui::ListBox(hwnd_, kList, 40, 200, 680, 262);

    ui::Button(hwnd_, "Alterar tecla", kChange, 40, 472, 170, 40);
    ui::Button(hwnd_, "Remover", kRemove, 222, 472, 150, 40);
    ui::Button(hwnd_, "Restaurar padrão", kReset, 384, 472, 200, 40);

    actionCombo_ = ui::ComboBox(hwnd_, kAction, 40, 528, 400, 240);
    for (std::size_t i = 0; i < kActionCount; ++i) {
        const std::string label(ActionLabel(static_cast<Action>(i)));
        const std::wstring wide = ui::ToWide(label);
        SendMessageW(actionCombo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(wide.c_str()));
    }
    SendMessageW(actionCombo_, CB_SETCURSEL, 0, 0);
    ui::Button(hwnd_, "Adicionar tecla", kAdd, 452, 526, 268, 40, true);

    message_ = ui::Label(hwnd_, "", kMessage, 40, 580, 680, 52, ui::Font(17), ui::kMuted);
    ui::Button(hwnd_, "Fechar", kClose, 580, 640, 140, 40);
}

LRESULT CALLBACK BindingsWindow::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
    }
    auto* self = reinterpret_cast<BindingsWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self) return self->OnMessage(hwnd, message, wParam, lParam);
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT BindingsWindow::OnMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_COMMAND:
            OnCommand(LOWORD(wParam), HIWORD(wParam));
            return 0;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            if (pending_ == Pending::None) break;
            if (wParam == VK_ESCAPE) {
                EndCapture();
                SetMessage("Captura cancelada.", ui::kMuted);
                return 0;
            }
            TryCapture(CapturedCode(wParam, lParam));
            return 0;
        case WM_LBUTTONDOWN:
            if (pending_ != Pending::None) TryCapture(kMouseLeft);
            return 0;
        case WM_RBUTTONDOWN:
            if (pending_ != Pending::None) TryCapture(kMouseRight);
            return 0;
        case WM_MBUTTONDOWN:
            if (pending_ != Pending::None) TryCapture(kMouseMiddle);
            return 0;
        case WM_XBUTTONDOWN:
            if (pending_ != Pending::None) TryCapture(HIWORD(wParam) == XBUTTON1 ? kMouseX1 : kMouseX2);
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
            EndCapture();
            hwnd_ = nullptr;
            return 0;
        default:
            break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

void BindingsWindow::OnCommand(int id, int notification) {
    switch (id) {
        case kProfile:
            if (notification == CBN_SELCHANGE) {
                EndCapture();
                const LRESULT selected = SendMessageW(profileCombo_, CB_GETCURSEL, 0, 0);
                LoadProfile(selected == kProfileDriving ? "driving" : "gta6");
            }
            break;
        case kChange: {
            const LRESULT row = SendMessageW(list_, LB_GETCURSEL, 0, 0);
            if (row == LB_ERR) {
                SetMessage("Escolha uma linha na lista primeiro.", ui::kBad);
                break;
            }
            StartCapture(Pending::ChangeRow, static_cast<std::size_t>(row), Action::Cross);
            break;
        }
        case kRemove:
            RemoveSelected();
            break;
        case kReset:
            ResetProfile();
            break;
        case kAdd: {
            const LRESULT selected = SendMessageW(actionCombo_, CB_GETCURSEL, 0, 0);
            if (selected == CB_ERR) break;
            StartCapture(Pending::AddAction, 0, static_cast<Action>(selected));
            break;
        }
        case kClose:
            DestroyWindow(hwnd_);
            break;
        default:
            break;
    }
}

void BindingsWindow::LoadProfile(const std::string& profileId) {
    profileId_ = profileId;
    const auto saved = app_.Settings().savedBindings.find(profileId_);
    const std::string savedText = saved == app_.Settings().savedBindings.end() ? std::string{} : saved->second;
    profile_ = ProfileWithBindings(profileId_, savedText);
    SendMessageW(profileCombo_, CB_SETCURSEL, profileId_ == "driving" ? kProfileDriving : kProfileGta, 0);
    FillList(0);
    SetMessage("", ui::kMuted);
}

void BindingsWindow::FillList(std::size_t selectRow) {
    SendMessageW(list_, LB_RESETCONTENT, 0, 0);
    for (const Binding& binding : profile_.bindings) {
        const std::string line = InputLabel(binding.input) + "    \u2192    " + std::string(ActionLabel(binding.action));
        SendMessageW(list_, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(ui::ToWide(line).c_str()));
    }
    if (!profile_.bindings.empty()) {
        SendMessageW(list_, LB_SETCURSEL, std::min(selectRow, profile_.bindings.size() - 1), 0);
    }
}

void BindingsWindow::StartCapture(Pending pending, std::size_t row, Action action) {
    pending_ = pending;
    pendingRow_ = row;
    pendingAction_ = action;
    SetPropW(hwnd_, ui::kRawKeysProp, reinterpret_cast<HANDLE>(static_cast<INT_PTR>(1)));
    SetFocus(hwnd_);
    SetCapture(hwnd_);
    SetMessage("Pressione a tecla ou clique com o mouse (botão esquerdo, direito ou do meio). Esc cancela.",
               ui::kWarn);
}

void BindingsWindow::EndCapture() {
    pending_ = Pending::None;
    if (GetCapture() == hwnd_) ReleaseCapture();
    if (hwnd_) RemovePropW(hwnd_, ui::kRawKeysProp);
}

void BindingsWindow::TryCapture(InputCode input) {
    const std::string_view reason = BindableInputError(input, static_cast<unsigned int>(app_.Settings().captureHotkeyVk));
    if (!reason.empty()) {
        SetMessage(reason, ui::kBad);
        return;
    }

    std::size_t row = 0;
    std::optional<Action> displaced;
    if (pending_ == Pending::ChangeRow) {
        displaced = SetBindingInput(profile_, pendingRow_, input);
        row = pendingRow_;
    } else {
        displaced = AddBinding(profile_, pendingAction_, input);
        const auto found = std::find_if(profile_.bindings.begin(), profile_.bindings.end(),
                                        [input](const Binding& b) { return b.input == input; });
        row = static_cast<std::size_t>(found - profile_.bindings.begin());
    }

    EndCapture();
    Persist();
    FillList(row);

    std::string text = "Tecla salva. Vale na próxima vez que você pressionar JOGAR.";
    if (displaced) {
        text = InputLabel(input) + " estava em \"" + std::string(ActionLabel(*displaced)) +
               "\" e foi removida de lá. " + text;
    }
    SetMessage(text, ui::kGood);
}

void BindingsWindow::Persist() {
    app_.Settings().savedBindings[profileId_] = SerializeBindingsInline(profile_);
    if (!app_.SaveSettings()) SetMessage("Não consegui salvar as teclas.", ui::kBad);
}

void BindingsWindow::ResetProfile() {
    EndCapture();
    profile_ = ProfileById(profileId_);
    app_.Settings().savedBindings.erase(profileId_);
    if (!app_.SaveSettings()) {
        SetMessage("Não consegui salvar as teclas.", ui::kBad);
        return;
    }
    FillList(0);
    SetMessage("Teclas restauradas para o padrão deste perfil.", ui::kGood);
}

void BindingsWindow::RemoveSelected() {
    const LRESULT row = SendMessageW(list_, LB_GETCURSEL, 0, 0);
    if (row == LB_ERR) {
        SetMessage("Escolha uma linha na lista primeiro.", ui::kBad);
        return;
    }
    if (!RemoveBinding(profile_, static_cast<std::size_t>(row))) {
        SetMessage("O perfil precisa de pelo menos uma tecla. Troque a tecla em vez de remover.", ui::kBad);
        return;
    }
    Persist();
    FillList(static_cast<std::size_t>(row));
    SetMessage("Tecla removida.", ui::kGood);
}

void BindingsWindow::SetMessage(std::string_view text, COLORREF color) {
    ui::SetText(message_, text);
    ui::SetColor(message_, color);
}

}  // namespace pskb
