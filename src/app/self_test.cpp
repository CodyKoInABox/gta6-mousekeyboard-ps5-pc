#include "app/self_test.h"

#include <string>
#include <vector>

#include "app/app_controller.h"
#include "app/main_window.h"
#include "core/windows_support.h"
#include "platform/windows_version.h"

namespace pskb {
namespace {

std::wstring ModuleDirectory() {
    std::vector<wchar_t> buffer(32768);
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    const std::wstring path(buffer.data(), length);
    return path.substr(0, path.find_last_of(L'\\') + 1);
}

bool BundledFilesPresent(const std::wstring& dir) {
    static const wchar_t* const kFiles[] = {
        L"ps5kb-cli.exe",
        L"LICENSE.txt",
        L"THIRD_PARTY_NOTICES.md",
        L"guia-configuracao.md",
        L"solucao-de-problemas.md",
    };
    for (const wchar_t* name : kFiles) {
        const DWORD attributes = GetFileAttributesW((dir + name).c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY)) return false;
    }
    return true;
}

}  // namespace

int RunSelfTest(HINSTANCE instance) {
    const WindowsVersion version = CurrentWindowsVersion();
    if (CheckWindowsSupport(version) == WindowsSupport::Unsupported) return kSelfTestUnsupportedWindows;

    if (!BundledFilesPresent(ModuleDirectory())) return kSelfTestMissingFile;

    AppController app;
    MainWindow window(instance, app);
    const bool created = window.Create() && window.Handle() != nullptr && IsWindow(window.Handle());
    if (created) DestroyWindow(window.Handle());
    return created ? kSelfTestOk : kSelfTestWindowFailed;
}

}  // namespace pskb
