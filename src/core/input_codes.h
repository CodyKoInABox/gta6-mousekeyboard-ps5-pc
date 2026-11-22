#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace pskb {

// Physical input identifiers. Keyboard codes are Windows virtual-key codes (VK_*);
// mouse buttons use a private range so they can never collide with a key.
using InputCode = std::uint16_t;

inline constexpr InputCode kMouseLeft = 0x101;
inline constexpr InputCode kMouseRight = 0x102;
inline constexpr InputCode kMouseMiddle = 0x103;
inline constexpr InputCode kMouseX1 = 0x104;
inline constexpr InputCode kMouseX2 = 0x105;

bool IsMouseButtonCode(InputCode code) noexcept;

// Human-readable names used in the settings file: "W", "Space", "LCtrl", "MouseLeft", "F8".
std::optional<InputCode> InputCodeFromName(std::string_view name);
std::string InputCodeToName(InputCode code);

}  // namespace pskb
