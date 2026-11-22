#pragma once

#include <cstdint>

#include "core/input_codes.h"

namespace pskb {

// Raw Input reports Shift, Ctrl and Alt as generic virtual keys. The scan code (Shift) or the
// E0 flag (Ctrl, Alt) tells the left and right keys apart. Other keys pass through unchanged.
InputCode KeyCodeFromRaw(std::uint16_t virtualKey, std::uint16_t makeCode, bool extended) noexcept;

}  // namespace pskb
