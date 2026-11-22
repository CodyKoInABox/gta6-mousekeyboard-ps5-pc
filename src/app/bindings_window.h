#pragma once

#include <windows.h>

#include <cstddef>
#include <string>
#include <string_view>

#include "app/app_controller.h"
#include "core/bindings.h"

namespace pskb {

// Teclas do perfil. Each row is one physical input driving one action. Changes are saved to
// settings as they happen and apply the next time the user presses JOGAR.
class BindingsWindow {
public:
    BindingsWindow(HINSTANCE instance, AppController& app);
    ~BindingsWindow();

    BindingsWindow(const BindingsWindow&) = delete;
    BindingsWindow& operator=(const BindingsWindow&) = delete;

    bool IsOpen() const noexcept { return hwnd_ != nullptr; }
    void Show();

private:
    enum class Pending : unsigned char { None, ChangeRow, AddAction };

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT OnMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    void Create(HINSTANCE instance);
    void OnCommand(int id, int notification);
    void LoadProfile(const std::string& profileId);
    void FillList(std::size_t selectRow);
    void StartCapture(Pending pending, std::size_t row, Action action);
    void EndCapture();
    void TryCapture(InputCode input);
    void Persist();
    void ResetProfile();
    void RemoveSelected();
    void SetMessage(std::string_view text, COLORREF color);

    HWND hwnd_ = nullptr;
    HWND list_ = nullptr;
    HWND profileCombo_ = nullptr;
    HWND actionCombo_ = nullptr;
    HWND message_ = nullptr;
    AppController& app_;
    std::string profileId_;
    Profile profile_;
    Pending pending_ = Pending::None;
    std::size_t pendingRow_ = 0;
    Action pendingAction_ = Action::Cross;
};

}  // namespace pskb
