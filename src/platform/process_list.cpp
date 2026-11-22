#include "platform/process_list.h"

#include <windows.h>
#include <tlhelp32.h>

#include <iterator>

namespace pskb {

std::optional<std::wstring> ProcessImagePath(unsigned long pid) {
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(pid));
    if (!process) return std::nullopt;
    wchar_t path[32768];
    DWORD length = static_cast<DWORD>(std::size(path));
    const bool ok = QueryFullProcessImageNameW(process, 0, path, &length) != 0;
    CloseHandle(process);
    if (!ok) return std::nullopt;
    return std::wstring(path, length);
}

std::vector<std::wstring> RunningProcessImagePaths() {
    std::vector<std::wstring> paths;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return paths;
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (auto path = ProcessImagePath(entry.th32ProcessID)) paths.push_back(std::move(*path));
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return paths;
}

}  // namespace pskb
