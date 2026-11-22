#pragma once

#include <windows.h>

#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "app/app_controller.h"
#include "backend/chiaki_discovery.h"
#include "core/account_id.h"
#include "core/app_model.h"
#include "core/pairing.h"

namespace pskb {

// Guided first-time setup in pt-BR. Modeless: the main screen stays usable behind it. Long
// operations (discovery, sign-in, pairing, the test connection) run on a worker thread so the
// window keeps repainting.
class SetupWizard {
public:
    SetupWizard(HINSTANCE instance, AppController& app, std::function<void()> onClosed);
    ~SetupWizard();

    SetupWizard(const SetupWizard&) = delete;
    SetupWizard& operator=(const SetupWizard&) = delete;

    bool IsOpen() const noexcept { return hwnd_ != nullptr; }
    void Show();

private:
    enum class Task : unsigned char { Probe, ManualProbe, SignIn, Pair, Test, FindVeyra };

    struct TaskResults {
        bool ok = false;
        std::string message;
        std::vector<ConsoleInfo> consoles;
        std::optional<ConsoleInfo> console;
        std::optional<AccountId> account;
        std::optional<std::wstring> veyra;
    };

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT OnMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    void Create(HINSTANCE instance);

    void ShowStep(WizardStep step);
    void ClearPage();
    void BuildPage();
    void BuildWelcome();
    void BuildConsole();
    void BuildRemotePlay();
    void BuildAccount();
    void BuildPin();
    void BuildTest();
    void BuildVeyra();
    void BuildDone();
    void FillConsoleList();
    void UpdateNavigation();
    void SetStatus(const std::string& text, COLORREF color);

    void OnCommand(int id, int notification);
    void Next();
    void Back();
    void Finish();
    void StartTask(Task task, std::function<void(TaskResults&)> work);
    void OnTaskDone();

    HWND Add(HWND control);

    HWND hwnd_ = nullptr;
    AppController& app_;
    std::function<void()> onClosed_;
    WizardStep step_ = WizardStep::Welcome;
    WizardProgress progress_;
    std::vector<HWND> page_;

    std::vector<ConsoleInfo> consoles_;
    std::optional<ConsoleInfo> chosen_;
    std::optional<AccountId> account_;
    std::optional<std::wstring> veyraPath_;
    bool launchVeyra_ = true;

    std::thread worker_;
    bool busy_ = false;
    Task running_ = Task::Probe;
    std::mutex resultMutex_;
    TaskResults results_;
};

}  // namespace pskb
