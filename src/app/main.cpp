#include <windows.h>

#include "app/app_controller.h"
#include "app/main_window.h"
#include "app/ui.h"
#include "core/windows_support.h"
#include "platform/windows_version.h"

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int) {
    SetProcessDPIAware();

    const pskb::WindowsVersion version = pskb::CurrentWindowsVersion();
    const pskb::WindowsSupport support = pskb::CheckWindowsSupport(version);
    if (support != pskb::WindowsSupport::Supported) {
        const std::wstring text = pskb::ui::ToWide(pskb::WindowsSupportMessage(version, support));
        if (support == pskb::WindowsSupport::Unsupported) {
            MessageBoxW(nullptr, text.c_str(), L"PS5 Keyboard Bridge", MB_ICONERROR);
            return 1;
        }
        MessageBoxW(nullptr, text.c_str(), L"PS5 Keyboard Bridge", MB_ICONWARNING);
    }

    if (HWND existing = pskb::MainWindow::FindRunningInstance()) {
        if (IsIconic(existing)) ShowWindow(existing, SW_RESTORE);
        ShowWindow(existing, SW_SHOW);
        SetForegroundWindow(existing);
        return 0;
    }

    pskb::AppController app;
    pskb::MainWindow window(instance, app);
    if (!window.Create()) {
        MessageBoxW(nullptr, L"Não consegui abrir a janela principal.", L"PS5 Keyboard Bridge", MB_ICONERROR);
        return 1;
    }
    window.Show();

    if (!app.HasPairing()) {
        window.OpenSetup();
    } else if (app.Settings().autoConnectOnLaunch) {
        window.Play();
    }

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (msg.hwnd) {
            HWND root = GetAncestor(msg.hwnd, GA_ROOT);
            if (root && !GetPropW(root, pskb::ui::kRawKeysProp) && IsDialogMessageW(root, &msg)) continue;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}
