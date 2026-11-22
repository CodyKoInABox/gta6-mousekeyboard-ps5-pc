#include "platform/veyra_locator.h"

#include <windows.h>
#include <shellapi.h>

#include <cstdlib>
#include <deque>
#include <filesystem>
#include <vector>

#include "core/app_model.h"
#include "platform/process_list.h"

#pragma comment(lib, "shell32.lib")

namespace pskb {
namespace {

namespace fs = std::filesystem;

constexpr int kMaxSearchDepth = 3;
constexpr std::size_t kMaxVisited = 6000;

std::optional<std::wstring> RunningVeyra() {
    for (const std::wstring& path : RunningProcessImagePaths()) {
        if (IsVeyraExecutableName(fs::path(path).filename().wstring())) return path;
    }
    return std::nullopt;
}

std::optional<std::wstring> SearchFolder(const fs::path& root) {
    std::error_code ec;
    if (!fs::is_directory(root, ec)) return std::nullopt;

    std::deque<std::pair<fs::path, int>> pending{{root, 0}};
    std::size_t visited = 0;
    while (!pending.empty() && visited < kMaxVisited) {
        const auto [dir, depth] = pending.front();
        pending.pop_front();
        ++visited;

        fs::directory_iterator it(dir, fs::directory_options::skip_permission_denied, ec);
        if (ec) continue;
        for (const fs::directory_entry& entry : it) {
            std::error_code entryEc;
            if (entry.is_regular_file(entryEc) && IsVeyraExecutableName(entry.path().filename().wstring())) {
                return entry.path().wstring();
            }
            if (depth + 1 < kMaxSearchDepth && entry.is_directory(entryEc) && !entryEc) {
                pending.emplace_back(entry.path(), depth + 1);
            }
        }
    }
    return std::nullopt;
}

fs::path EnvPath(const wchar_t* name) {
    wchar_t* value = nullptr;
    size_t length = 0;
    fs::path result;
    if (_wdupenv_s(&value, &length, name) == 0 && value && *value) result = value;
    std::free(value);
    return result;
}

struct WindowSearch {
    std::optional<HWND> found;
};

BOOL CALLBACK FindVeyraWindow(HWND window, LPARAM param) {
    auto* search = reinterpret_cast<WindowSearch*>(param);
    if (!IsWindowVisible(window)) return TRUE;
    DWORD pid = 0;
    GetWindowThreadProcessId(window, &pid);
    if (pid == 0) return TRUE;
    const auto path = ProcessImagePath(pid);
    if (path && IsVeyraExecutableName(fs::path(*path).filename().wstring())) {
        search->found = window;
        return FALSE;
    }
    return TRUE;
}

}  // namespace

std::optional<std::wstring> FindVeyraExecutable() {
    if (auto running = RunningVeyra()) return running;

    const fs::path home = EnvPath(L"USERPROFILE");
    const std::vector<fs::path> roots = {
        EnvPath(L"LOCALAPPDATA"),
        EnvPath(L"ProgramFiles"),
        EnvPath(L"ProgramFiles(x86)"),
        home / L"Desktop",
        home / L"Downloads",
    };
    for (const fs::path& root : roots) {
        if (root.empty()) continue;
        if (auto found = SearchFolder(root)) return found;
    }
    return std::nullopt;
}

bool LaunchOrFocusVeyra(const std::wstring& exePath) {
    WindowSearch search;
    EnumWindows(&FindVeyraWindow, reinterpret_cast<LPARAM>(&search));
    if (search.found) {
        HWND window = *search.found;
        if (IsIconic(window)) ShowWindow(window, SW_RESTORE);
        SetForegroundWindow(window);
        return true;
    }
    if (exePath.empty()) return false;

    const fs::path folder = fs::path(exePath).parent_path();
    const auto result = reinterpret_cast<INT_PTR>(
        ShellExecuteW(nullptr, L"open", exePath.c_str(), nullptr, folder.c_str(), SW_SHOWNORMAL));
    return result > 32;
}

}  // namespace pskb
