#include "core/input_codes.h"

#include <array>
#include <cctype>

namespace pskb {
namespace {

struct NamedCode {
    std::string_view name;
    InputCode code;
};

// Windows virtual-key values, so the Win32 layer can pass them straight through.
constexpr std::array<NamedCode, 40> kNamedCodes{{
    {"Space", 0x20},     {"Tab", 0x09},       {"Enter", 0x0D},      {"Esc", 0x1B},
    {"Backspace", 0x08}, {"LShift", 0xA0},    {"RShift", 0xA1},     {"LCtrl", 0xA2},
    {"RCtrl", 0xA3},     {"LAlt", 0xA4},      {"RAlt", 0xA5},       {"Up", 0x26},
    {"Down", 0x28},      {"Left", 0x25},      {"Right", 0x27},      {"Insert", 0x2D},
    {"Delete", 0x2E},    {"Home", 0x24},      {"End", 0x23},        {"PageUp", 0x21},
    {"PageDown", 0x22},  {"F1", 0x70},        {"F2", 0x71},         {"F3", 0x72},
    {"F4", 0x73},        {"F5", 0x74},        {"F6", 0x75},         {"F7", 0x76},
    {"F8", 0x77},        {"F9", 0x78},        {"F10", 0x79},        {"F11", 0x7A},
    {"F12", 0x7B},       {"MouseLeft", kMouseLeft},     {"MouseRight", kMouseRight},
    {"MouseMiddle", kMouseMiddle}, {"MouseX1", kMouseX1}, {"MouseX2", kMouseX2},
    {"Numpad0", 0x60},   {"Numpad1", 0x61},
}};

bool EqualsIgnoreCase(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::toupper(static_cast<unsigned char>(a[i])) !=
            std::toupper(static_cast<unsigned char>(b[i])))
            return false;
    }
    return true;
}

}  // namespace

bool IsMouseButtonCode(InputCode code) noexcept {
    return code >= kMouseLeft && code <= kMouseX2;
}

std::optional<InputCode> InputCodeFromName(std::string_view name) {
    if (name.size() == 1) {
        const auto c = static_cast<unsigned char>(std::toupper(static_cast<unsigned char>(name[0])));
        if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) return static_cast<InputCode>(c);
        return std::nullopt;
    }
    for (const auto& entry : kNamedCodes) {
        if (EqualsIgnoreCase(entry.name, name)) return entry.code;
    }
    return std::nullopt;
}

std::string InputCodeToName(InputCode code) {
    for (const auto& entry : kNamedCodes) {
        if (entry.code == code) return std::string(entry.name);
    }
    if ((code >= 'A' && code <= 'Z') || (code >= '0' && code <= '9')) {
        return std::string(1, static_cast<char>(code));
    }
    return "Key" + std::to_string(code);
}

}  // namespace pskb
