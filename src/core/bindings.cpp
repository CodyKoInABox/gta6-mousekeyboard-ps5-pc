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

Profile ProfileById(std::string_view id) {
    return id == "driving" ? DrivingProfile() : GtaProfile();
}

Profile ProfileWithBindings(std::string_view id, std::string_view savedBindings) {
    Profile profile = ProfileById(id);
    if (!savedBindings.empty()) {
        Profile saved = profile;
        if (ParseBindingsInline(savedBindings, saved)) profile = std::move(saved);
    }
    return profile;
}

std::string SerializeBindingsInline(const Profile& profile) {
    std::string out;
    for (const auto& b : profile.bindings) {
        if (!out.empty()) out += ';';
        out += InputCodeToName(b.input);
        out += '=';
        out += ActionToName(b.action);
    }
    return out;
}

bool ParseBindingsInline(std::string_view text, Profile& profile, std::string* error) {
    std::string lines(text);
    std::replace(lines.begin(), lines.end(), ';', '\n');
    Profile candidate = profile;
    if (!ParseBindings(lines, candidate, error)) return false;
    const std::string conflict = ValidateBindings(candidate);
    if (!conflict.empty()) {
        if (error) *error += conflict + "\n";
        return false;
    }
    profile = std::move(candidate);
    return true;
}

std::string_view BindableInputError(InputCode input, unsigned int captureHotkeyVk) noexcept {
    if (input == 0x1B) return "Esc é reservado para cancelar e não pode ser usado.";
    if (input == 0x5B || input == 0x5C) return "A tecla Windows não pode ser usada.";
    if (input == captureHotkeyVk) return "Essa tecla liga e desliga a captura. Escolha outra.";
    if (input == kMouseX1 || input == kMouseX2) return "Botões laterais do mouse não são suportados.";
    if (!IsMouseButtonCode(input)) {
        const auto name = InputCodeFromName(InputCodeToName(input));
        if (!name || *name != input) return "Essa tecla não tem nome estável e não pode ser salva.";
    }
    return {};
}

std::string_view ActionLabel(Action action) noexcept {
    switch (action) {
        case Action::Cross: return "X";
        case Action::Circle: return "Círculo";
        case Action::Square: return "Quadrado";
        case Action::Triangle: return "Triângulo";
        case Action::DpadUp: return "Direcional: cima";
        case Action::DpadDown: return "Direcional: baixo";
        case Action::DpadLeft: return "Direcional: esquerda";
        case Action::DpadRight: return "Direcional: direita";
        case Action::L1: return "L1";
        case Action::R1: return "R1";
        case Action::L2: return "L2 (gatilho esquerdo, mira)";
        case Action::R2: return "R2 (gatilho direito, tiro)";
        case Action::L3: return "L3 (clique do analógico esquerdo)";
        case Action::R3: return "R3 (clique do analógico direito)";
        case Action::Options: return "Options";
        case Action::Share: return "Share";
        case Action::Touchpad: return "Touchpad";
        case Action::Ps: return "Botão PS";
        case Action::LeftStickUp: return "Analógico esquerdo: cima";
        case Action::LeftStickDown: return "Analógico esquerdo: baixo";
        case Action::LeftStickLeft: return "Analógico esquerdo: esquerda";
        case Action::LeftStickRight: return "Analógico esquerdo: direita";
    }
    return "Desconhecido";
}

std::string InputLabel(InputCode input) {
    switch (input) {
        case kMouseLeft: return "Mouse: botão esquerdo";
        case kMouseRight: return "Mouse: botão direito";
        case kMouseMiddle: return "Mouse: botão do meio";
        default: return InputCodeToName(input);
    }
}

std::optional<Action> SetBindingInput(Profile& profile, std::size_t index, InputCode input) {
    if (index >= profile.bindings.size()) return std::nullopt;
    if (profile.bindings[index].input == input) return std::nullopt;

    const Action action = profile.bindings[index].action;
    std::optional<Action> displaced;
    std::size_t row = index;
    for (std::size_t i = 0; i < profile.bindings.size();) {
        if (profile.bindings[i].input != input) {
            ++i;
            continue;
        }
        if (profile.bindings[i].action != action) displaced = profile.bindings[i].action;
        profile.bindings.erase(profile.bindings.begin() + static_cast<std::ptrdiff_t>(i));
        if (i < row) --row;
    }
    profile.bindings[row].input = input;
    return displaced;
}

std::optional<Action> AddBinding(Profile& profile, Action action, InputCode input) {
    std::optional<Action> displaced;
    for (std::size_t i = 0; i < profile.bindings.size();) {
        if (profile.bindings[i].input != input) {
            ++i;
            continue;
        }
        if (profile.bindings[i].action == action) return std::nullopt;
        displaced = profile.bindings[i].action;
        profile.bindings.erase(profile.bindings.begin() + static_cast<std::ptrdiff_t>(i));
    }
    profile.bindings.push_back({input, action});
    return displaced;
}

bool RemoveBinding(Profile& profile, std::size_t index) {
    if (profile.bindings.size() <= 1 || index >= profile.bindings.size()) return false;
    profile.bindings.erase(profile.bindings.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

}  // namespace pskb
