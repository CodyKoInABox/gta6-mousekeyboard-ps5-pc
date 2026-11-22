#pragma once

#include "core/windows_support.h"

namespace pskb {

// Reads the real OS version. RtlGetVersion is used because GetVersionEx and
// IsWindows10OrGreater report older versions unless the app carries a compatibility manifest.
WindowsVersion CurrentWindowsVersion();

}  // namespace pskb
