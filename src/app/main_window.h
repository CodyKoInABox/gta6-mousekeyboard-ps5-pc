#pragma once

#include <windows.h>
#include <shellapi.h>

#include <memory>
#include <string>

#include "app/app_controller.h"
#include "app/setup_wizard.h"
#include "app/settings_window.h"

namespace pskb {

// Tela principal: status, JOGAR/PARAR, and the tray icon. Child windows (wizard, settings) are
// owned by this window and released from the timer after they close, never from inside their
// own message handlers.
class MainWindow {
public:
    MainWindow(HINSTANCE instance, AppController& app);
    ~MainWindow();

    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;

    bool Create();
    HWND Handle() const noexcept { return hwnd_; }
    void Show();
    void OpenSetup();
    void OpenSettings();
    void Play();

    static HWND FindRunningInstance();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT OnMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    void Layout();
    void Refresh();
    void RefreshPairing();
    void ReapClosedWindows();
    void OnCommand(int id);
    void Stop();
    void HideToTray();
    void RestoreFromTray();
    void AddTrayIcon();
    void RemoveTrayIcon();
    void ShowTrayMenu();
    void Quit();

    HINSTANCE instance_;
    AppController& app_;
    HWND hwnd_ = nullptr;
    HWND statusLabel_ = nullptr;
    HWND hintLabel_ = nullptr;
    HWND pairLabel_ = nullptr;
    HWND veyraLabel_ = nullptr;
    HWND captureLabel_ = nullptr;
    HWND messageLabel_ = nullptr;
    HWND playButton_ = nullptr;
    HWND stopButton_ = nullptr;
    std::unique_ptr<SetupWizard> wizard_;
    std::unique_ptr<SettingsWindow> settings_;

    bool paired_ = false;
    std::string pairedName_;
    std::string lastError_;
    bool quitting_ = false;
    bool closing_ = false;
    bool trayVisible_ = false;
    NOTIFYICONDATAW tray_{};
};

}  // namespace pskb
