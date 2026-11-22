#pragma once

#include <windows.h>

namespace pskb {

// Runs `PS5KeyboardBridge.exe --self-test`. Checks the Windows version, the bundled files next to
// the exe, the saved settings, and that the main window can be created. The window is never shown,
// no link or capture starts, and nothing is written to disk. Returns 0 when everything passed.
// scripts/build-release.ps1 runs this on the installed and portable builds.
int RunSelfTest(HINSTANCE instance);

inline constexpr int kSelfTestOk = 0;
inline constexpr int kSelfTestUnsupportedWindows = 2;
inline constexpr int kSelfTestMissingFile = 3;
inline constexpr int kSelfTestWindowFailed = 4;

}  // namespace pskb
