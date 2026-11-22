#include "platform/windows_version.h"

#include <windows.h>

namespace pskb {

WindowsVersion CurrentWindowsVersion() {
    WindowsVersion version;

    using RtlGetVersionFn = LONG(WINAPI*)(PRTL_OSVERSIONINFOW);
    const HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    const auto rtlGetVersion =
        ntdll ? reinterpret_cast<RtlGetVersionFn>(GetProcAddress(ntdll, "RtlGetVersion")) : nullptr;
    if (rtlGetVersion) {
        RTL_OSVERSIONINFOW info{};
        info.dwOSVersionInfoSize = sizeof(info);
        if (rtlGetVersion(&info) == 0) {
            version.major = info.dwMajorVersion;
            version.minor = info.dwMinorVersion;
            version.build = info.dwBuildNumber;
        }
    }

    SYSTEM_INFO native{};
    GetNativeSystemInfo(&native);
    version.is64Bit = native.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
                      native.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64;
    return version;
}

}  // namespace pskb
