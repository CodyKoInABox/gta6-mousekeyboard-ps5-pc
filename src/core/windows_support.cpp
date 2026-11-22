#include "core/windows_support.h"

namespace pskb {

WindowsSupport CheckWindowsSupport(const WindowsVersion& version) noexcept {
    if (!version.is64Bit) return WindowsSupport::Unsupported;
    if (version.major < 10) return WindowsSupport::Unsupported;
    if (version.major == 10 && version.build < kMinTestedWindows10Build) return WindowsSupport::Untested;
    return WindowsSupport::Supported;
}

std::string WindowsSupportMessage(const WindowsVersion& version, WindowsSupport support) {
    switch (support) {
        case WindowsSupport::Supported:
            return {};
        case WindowsSupport::Untested:
            return "Windows 10 build " + std::to_string(version.build) +
                   " é anterior ao 1809 (build " + std::to_string(kMinTestedWindows10Build) +
                   ") e não foi testado. O app pode funcionar, mas atualize o Windows se algo falhar.";
        case WindowsSupport::Unsupported:
            return "Este app precisa de Windows 10 ou 11 de 64 bits.";
    }
    return {};
}

}  // namespace pskb
