#pragma once

#include <cstdlib>
#include <filesystem>

namespace pskb {

// %LOCALAPPDATA%\PS5KeyboardBridge: per-user, no admin rights, never roams to other PCs.
inline std::filesystem::path DataDirectory() {
    std::filesystem::path base;
    wchar_t* local = nullptr;
    size_t length = 0;
    if (_wdupenv_s(&local, &length, L"LOCALAPPDATA") == 0 && local && *local) {
        base = local;
    } else {
        base = std::filesystem::temp_directory_path();
    }
    std::free(local);
    return base / L"PS5KeyboardBridge";
}

inline std::filesystem::path PairingFilePath() { return DataDirectory() / L"pairing.bin"; }
inline std::filesystem::path SettingsFilePath() { return DataDirectory() / L"settings.ini"; }

}  // namespace pskb
