#include "core/bindings.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <map>
#include <sstream>

namespace pskb {
namespace {

struct ActionName {
    Action action;
    std::string_view name;
};

constexpr std::array<ActionName, kActionCount> kActionNames{{
    {Action::Cross, "cross"},
    {Action::Circle, "circle"},
    {Action::Square, "square"},
    {Action::Triangle, "triangle"},
    {Action::DpadUp, "dpad_up"},
    {Action::DpadDown, "dpad_down"},
    {Action::DpadLeft, "dpad_left"},
    {Action::DpadRight, "dpad_right"},
    {Action::L1, "l1"},
    {Action::R1, "r1"},
    {Action::L2, "l2"},
    {Action::R2, "r2"},
    {Action::L3, "l3"},
    {Action::R3, "r3"},
    {Action::Options, "options"},
    {Action::Share, "share"},
    {Action::Touchpad, "touchpad"},
    {Action::Ps, "ps"},
    {Action::LeftStickUp, "left_stick_up"},
    {Action::LeftStickDown, "left_stick_down"},
    {Action::LeftStickLeft, "left_stick_left"},
    {Action::LeftStickRight, "left_stick_right"},
}};

std::string Trim(std::string_view s) {
    size_t begin = 0;
    size_t end = s.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(s[begin]))) ++begin;
    while (end > begin && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return std::string(s.substr(begin, end - begin));
}

std::string Lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

Profile MakeProfile(std::string id, std::vector<Binding> bindings) {
    Profile p;
    p.id = std::move(id);
    p.bindings = std::move(bindings);
    return p;
}

}  // namespace

std::string_view ActionToName(Action action) noexcept {
    for (const auto& entry : kActionNames) {
        if (entry.action == action) return entry.name;
    }
    return "none";
}

std::optional<Action> ActionFromName(std::string_view name) {
    const std::string wanted = Lower(Trim(name));
    for (const auto& entry : kActionNames) {
        if (entry.name == wanted) return entry.action;
    }
    return std::nullopt;
}

Profile GtaProfile() {
    Profile p = MakeProfile(
        "gta6",
        {
            {'W', Action::LeftStickUp},
            {'A', Action::LeftStickLeft},
            {'S', Action::LeftStickDown},
            {'D', Action::LeftStickRight},
            {kMouseLeft, Action::R2},
            {kMouseRight, Action::L2},
            {0x20, Action::Cross},             // Space
            {0xA2, Action::Circle},            // Left Ctrl
            {'E', Action::Triangle},
            {'R', Action::Square},
            {0xA0, Action::L3},                // Left Shift
            {kMouseMiddle, Action::R3},
            {'Q', Action::L1},
            {'F', Action::R1},
            {0x09, Action::Touchpad},          // Tab
            {0x0D, Action::Options},           // Enter
            {0x26, Action::DpadUp},
            {0x28, Action::DpadDown},
            {0x25, Action::DpadLeft},
            {0x27, Action::DpadRight},
            {0x08, Action::Share},             // Backspace
        });
    return p;
}

Profile DrivingProfile() {
    Profile p = MakeProfile(
        "driving",
        {
            {'A', Action::LeftStickLeft},
            {'D', Action::LeftStickRight},
            {'W', Action::R2},
            {'S', Action::L2},
            {kMouseLeft, Action::R2},
            {kMouseRight, Action::L2},
            {0x20, Action::Cross},             // Space
            {0xA2, Action::Circle},            // Left Ctrl
            {'E', Action::Triangle},
            {'R', Action::Square},
            {0xA0, Action::L3},
            {kMouseMiddle, Action::R3},
            {'Q', Action::L1},
            {'F', Action::R1},
            {0x09, Action::Touchpad},
            {0x0D, Action::Options},
            {0x26, Action::DpadUp},
            {0x28, Action::DpadDown},
            {0x25, Action::DpadLeft},
            {0x27, Action::DpadRight},
            {0x08, Action::Share},
        });
    p.stickRampMs = 250.0f;
    p.triggerRampMs = 120.0f;
    p.aim.sensitivityX = 0.7f;
    p.aim.sensitivityY = 0.7f;
    return p;
}

std::string SerializeBindings(const Profile& profile) {
    std::ostringstream out;
    for (const auto& b : profile.bindings) {
        out << InputCodeToName(b.input) << '=' << ActionToName(b.action) << '\n';
    }
    return out.str();
}

bool ParseBindings(std::string_view text, Profile& profile, std::string* error) {
    std::vector<Binding> parsed;
    bool ok = true;
    std::istringstream in{std::string(text)};
    std::string raw;
    int lineNo = 0;
    while (std::getline(in, raw)) {
        ++lineNo;
        const std::string line = Trim(raw);
        if (line.empty() || line[0] == '#') continue;
        const auto eq = line.find('=');
        auto fail = [&](const std::string& why) {
            ok = false;
            if (error) *error += "line " + std::to_string(lineNo) + ": " + why + "\n";
        };
        if (eq == std::string::npos) {
            fail("expected input=action");
            continue;
        }
        const auto input = InputCodeFromName(Trim(std::string_view(line).substr(0, eq)));
        if (!input) {
            fail("unknown input '" + line.substr(0, eq) + "'");
            continue;
        }
        const auto action = ActionFromName(std::string_view(line).substr(eq + 1));
        if (!action) {
            fail("unknown action '" + line.substr(eq + 1) + "'");
            continue;
        }
        parsed.push_back({*input, *action});
    }
    if (ok && parsed.empty()) {
        if (error) *error += "no bindings found\n";
        return false;
    }
    if (ok) profile.bindings = std::move(parsed);
    return ok;
}

std::string ValidateBindings(const Profile& profile) {
    std::map<InputCode, Action> seen;
    for (const auto& b : profile.bindings) {
        const auto it = seen.find(b.input);
        if (it != seen.end() && it->second != b.action) {
            return InputCodeToName(b.input) + " is bound to both " +
                   std::string(ActionToName(it->second)) + " and " + std::string(ActionToName(b.action));
        }
        seen[b.input] = b.action;
    }
    return {};
}

}  // namespace pskb
