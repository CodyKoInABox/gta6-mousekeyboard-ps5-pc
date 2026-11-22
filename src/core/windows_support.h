#pragma once

#include <string>

namespace pskb {

struct WindowsVersion {
    unsigned int major = 0;
    unsigned int minor = 0;
    unsigned int build = 0;
    bool is64Bit = false;
};

enum class WindowsSupport {
    Supported,    // Windows 11, or Windows 10 build 17763 (version 1809) and newer
    Untested,     // older Windows 10 builds: may work, not validated
    Unsupported,  // Windows 8.1 and older, or a 32-bit system
};

// Assumption: Windows 10 1809 (build 17763) is the oldest build the app is validated on. The
// Windows 10 build used for development (19045) is newer than that. Revisit when tested builds
// are recorded in docs/resultados-testes.md.
inline constexpr unsigned int kMinTestedWindows10Build = 17763;

WindowsSupport CheckWindowsSupport(const WindowsVersion& version) noexcept;

// Portuguese text for the wizard and startup check. Empty when Supported.
std::string WindowsSupportMessage(const WindowsVersion& version, WindowsSupport support);

}  // namespace pskb
