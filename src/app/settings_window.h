#pragma once

#include <windows.h>

#include <functional>
#include <memory>
#include <vector>

#include "app/app_controller.h"
#include "app/bindings_window.h"

namespace pskb {

// Configurações. Modeless top-level window. Advanced aim tuning stays hidden until asked for.
class SettingsWindow {
public:
    SettingsWindow(HINSTANCE instance, AppController& app, std::function<void()> onClosed,
                   std::function<void()> onSetup);
    ~SettingsWindow();

    SettingsWindow(const SettingsWindow&) = delete;
    SettingsWindow& operator=(const SettingsWindow&) = delete;

    bool IsOpen() const noexcept { return hwnd_ != nullptr; }
    void Show();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT OnMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    void Create(HINSTANCE instance);
    void Load();
    void OnCommand(int id, int notification);
    bool Save();
    void ToggleAdvanced();
    void ApplyLayout();
    void OpenBindings();
    void ForgetPairing();
    void SetMessage(const char* text, COLORREF color);

    HWND hwnd_ = nullptr;
    HINSTANCE instance_ = nullptr;
    AppController& app_;
    std::function<void()> onClosed_;
    std::function<void()> onSetup_;
    std::vector<HWND> advanced_;
    std::unique_ptr<BindingsWindow> bindings_;
};

}  // namespace pskb
