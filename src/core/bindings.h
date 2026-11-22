#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "core/input_codes.h"
#include "core/mouse_to_stick.h"

namespace pskb {

// Things a physical input can drive. Mouse movement is not an action: it always feeds the
// right stick through MouseToStick.
enum class Action : unsigned char {
    Cross,
    Circle,
    Square,
    Triangle,
    DpadUp,
    DpadDown,
    DpadLeft,
    DpadRight,
    L1,
    R1,
    L2,
    R2,
    L3,
    R3,
    Options,
    Share,
    Touchpad,
    Ps,
    LeftStickUp,
    LeftStickDown,
    LeftStickLeft,
    LeftStickRight,
};

inline constexpr std::size_t kActionCount = 22;

std::string_view ActionToName(Action action) noexcept;
std::optional<Action> ActionFromName(std::string_view name);

struct Binding {
    InputCode input = 0;
    Action action = Action::Cross;
};

struct Profile {
    std::string id;                  // "gta6" or "driving" or a user-defined name
    std::vector<Binding> bindings;
    float stickRampMs = 0.0f;        // time for WASD to reach full left-stick deflection; 0 = instant
    float triggerRampMs = 0.0f;      // time for L2/R2 to reach full press; 0 = instant
    AimSettings aim;
};

// Proposed GTA 6 defaults from the project brief. These are not guaranteed in-game actions.
Profile GtaProfile();

// Optional driving profile: smooth steering ramp and analog throttle/brake on W/S.
Profile DrivingProfile();

// Settings-file form: one "input=action" pair per line (see settings.h).
std::string SerializeBindings(const Profile& profile);

// Parses "input=action" lines, replacing the profile's bindings. Unknown names are reported
// through `error` and skipped; the function returns false if any line was invalid.
bool ParseBindings(std::string_view text, Profile& profile, std::string* error = nullptr);

// Empty when valid. A physical input may drive only one action; one action may have
// several inputs (for example W and Up both driving the left stick).
std::string ValidateBindings(const Profile& profile);

}  // namespace pskb
