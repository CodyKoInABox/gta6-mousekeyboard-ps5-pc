#pragma once

#include <filesystem>
#include <map>
#include <string>

#include "core/mouse_to_stick.h"

namespace pskb {

// Non-secret user preferences. Pairing secrets live in SecureStore, never here.
struct AppSettings {
    std::string language = "pt-BR";
    bool darkTheme = true;
    bool autoConnectOnLaunch = false;
    bool autoReconnect = true;
    bool minimizeToTray = true;
    bool launchVeyra = true;
    std::string veyraPath;            // empty = detect automatically
    std::string profileId = "gta6";   // "gta6" or "driving"
    unsigned int captureHotkeyVk = 0x77;  // F8
    AimSettings aim;
    // Profile id -> inline bindings text (see SerializeBindingsInline). A profile missing
    // here uses its built-in defaults.
    std::map<std::string, std::string> savedBindings;
};

// Plain key=value text, one per line, '#' comments. Unknown keys are ignored so older
// builds can read newer files.
std::string SerializeSettings(const AppSettings& settings);
bool ParseSettings(const std::string& text, AppSettings& settings);

bool LoadSettingsFile(const std::filesystem::path& path, AppSettings& settings);
bool SaveSettingsFile(const std::filesystem::path& path, const AppSettings& settings);

}  // namespace pskb
