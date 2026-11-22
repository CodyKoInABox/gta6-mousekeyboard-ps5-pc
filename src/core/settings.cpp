#include "core/settings.h"

#include <charconv>
#include <cmath>
#include <fstream>
#include <map>
#include <sstream>
#include <string_view>
#include <system_error>

namespace pskb {
namespace {

std::string Trim(const std::string& s) {
    const auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return {};
    const auto e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

std::string BoolText(bool v) { return v ? "true" : "false"; }

bool ParseBool(const std::string& v, bool& out) {
    if (v == "true" || v == "1") { out = true; return true; }
    if (v == "false" || v == "0") { out = false; return true; }
    return false;
}

bool ParseFloat(const std::string& v, float& out) {
    const char* begin = v.data();
    const char* end = v.data() + v.size();
    float parsed = 0.0f;
    const auto result = std::from_chars(begin, end, parsed);
    if (result.ec != std::errc() || result.ptr != end || !std::isfinite(parsed)) return false;
    out = parsed;
    return true;
}

bool ParseUInt(const std::string& v, unsigned int& out) {
    unsigned int parsed = 0;
    const auto result = std::from_chars(v.data(), v.data() + v.size(), parsed);
    if (result.ec != std::errc() || result.ptr != v.data() + v.size()) return false;
    out = parsed;
    return true;
}

}  // namespace

std::string SerializeSettings(const AppSettings& s) {
    std::ostringstream out;
    out << "# PS5 Keyboard Bridge settings. Safe to edit with Notepad; pairing is stored separately.\n";
    out << "language=" << s.language << '\n';
    out << "dark_theme=" << BoolText(s.darkTheme) << '\n';
    out << "auto_connect_on_launch=" << BoolText(s.autoConnectOnLaunch) << '\n';
    out << "auto_reconnect=" << BoolText(s.autoReconnect) << '\n';
    out << "minimize_to_tray=" << BoolText(s.minimizeToTray) << '\n';
    out << "launch_veyra=" << BoolText(s.launchVeyra) << '\n';
    out << "veyra_path=" << s.veyraPath << '\n';
    out << "profile=" << s.profileId << '\n';
    out << "capture_hotkey_vk=" << s.captureHotkeyVk << '\n';
    out << "aim.sensitivity_x=" << s.aim.sensitivityX << '\n';
    out << "aim.sensitivity_y=" << s.aim.sensitivityY << '\n';
    out << "aim.invert_y=" << BoolText(s.aim.invertY) << '\n';
    out << "aim.deadzone=" << s.aim.deadzone << '\n';
    out << "aim.anti_deadzone=" << s.aim.antiDeadzone << '\n';
    out << "aim.exponential_curve=" << BoolText(s.aim.exponentialCurve) << '\n';
    out << "aim.curve_exponent=" << s.aim.curveExponent << '\n';
    out << "aim.acceleration=" << s.aim.acceleration << '\n';
    out << "aim.smoothing_ms=" << s.aim.smoothingMs << '\n';
    out << "aim.recenter_ms=" << s.aim.recenterMs << '\n';
    out << "aim.window_ms=" << s.aim.windowMs << '\n';
    out << "aim.full_counts_per_sec=" << s.aim.fullDeflectionCountsPerSec << '\n';
    out << "aim.aim_sensitivity_x=" << s.aim.aimSensitivityX << '\n';
    out << "aim.aim_sensitivity_y=" << s.aim.aimSensitivityY << '\n';
    for (const auto& [profileId, text] : s.savedBindings) {
        out << "bindings." << profileId << '=' << text << '\n';
    }
    return out.str();
}

bool ParseSettings(const std::string& text, AppSettings& s) {
    std::map<std::string, std::string> kv;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        const std::string t = Trim(line);
        if (t.empty() || t[0] == '#') continue;
        const auto eq = t.find('=');
        if (eq == std::string::npos) return false;
        kv[Trim(t.substr(0, eq))] = Trim(t.substr(eq + 1));
    }

    AppSettings out = s;
    bool ok = true;
    auto boolKey = [&](const char* key, bool& field) {
        const auto it = kv.find(key);
        if (it != kv.end() && !ParseBool(it->second, field)) ok = false;
    };
    auto floatKey = [&](const char* key, float& field) {
        const auto it = kv.find(key);
        if (it != kv.end() && !ParseFloat(it->second, field)) ok = false;
    };

    if (kv.count("language")) out.language = kv["language"];
    boolKey("dark_theme", out.darkTheme);
    boolKey("auto_connect_on_launch", out.autoConnectOnLaunch);
    boolKey("auto_reconnect", out.autoReconnect);
    boolKey("minimize_to_tray", out.minimizeToTray);
    boolKey("launch_veyra", out.launchVeyra);
    if (kv.count("veyra_path")) out.veyraPath = kv["veyra_path"];
    if (kv.count("profile")) out.profileId = kv["profile"];
    if (kv.count("capture_hotkey_vk") && !ParseUInt(kv["capture_hotkey_vk"], out.captureHotkeyVk)) ok = false;

    floatKey("aim.sensitivity_x", out.aim.sensitivityX);
    floatKey("aim.sensitivity_y", out.aim.sensitivityY);
    boolKey("aim.invert_y", out.aim.invertY);
    floatKey("aim.deadzone", out.aim.deadzone);
    floatKey("aim.anti_deadzone", out.aim.antiDeadzone);
    boolKey("aim.exponential_curve", out.aim.exponentialCurve);
    floatKey("aim.curve_exponent", out.aim.curveExponent);
    floatKey("aim.acceleration", out.aim.acceleration);
    floatKey("aim.smoothing_ms", out.aim.smoothingMs);
    floatKey("aim.recenter_ms", out.aim.recenterMs);
    floatKey("aim.window_ms", out.aim.windowMs);
    floatKey("aim.full_counts_per_sec", out.aim.fullDeflectionCountsPerSec);
    floatKey("aim.aim_sensitivity_x", out.aim.aimSensitivityX);
    floatKey("aim.aim_sensitivity_y", out.aim.aimSensitivityY);

    constexpr std::string_view kBindingsPrefix = "bindings.";
    out.savedBindings.clear();
    for (const auto& [key, value] : kv) {
        if (key.compare(0, kBindingsPrefix.size(), kBindingsPrefix) != 0) continue;
        const std::string profileId = key.substr(kBindingsPrefix.size());
        if (profileId.empty()) continue;
        out.savedBindings[profileId] = value;
    }

    if (!ok) return false;
    s = out;
    return true;
}

bool LoadSettingsFile(const std::filesystem::path& path, AppSettings& settings) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::ostringstream buf;
    buf << in.rdbuf();
    return ParseSettings(buf.str(), settings);
}

bool SaveSettingsFile(const std::filesystem::path& path, const AppSettings& settings) {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    if (ec) return false;
    const std::filesystem::path temp = path.string() + ".tmp";
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        out << SerializeSettings(settings);
        if (!out) return false;
    }
    std::filesystem::rename(temp, path, ec);
    if (ec) {
        std::filesystem::remove(temp, ec);
        return false;
    }
    return true;
}

}  // namespace pskb
