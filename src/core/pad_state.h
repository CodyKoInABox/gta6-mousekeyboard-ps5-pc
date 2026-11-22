#pragma once

#include <cstdint>

namespace pskb {

// Bit positions match the DualSense button bitmask used by the Remote Play protocol,
// so the backend can pass the mask through unchanged.
enum PadButton : std::uint32_t {
    PadCross = 1u << 0,
    PadCircle = 1u << 1,
    PadSquare = 1u << 2,
    PadTriangle = 1u << 3,
    PadDpadLeft = 1u << 4,
    PadDpadRight = 1u << 5,
    PadDpadUp = 1u << 6,
    PadDpadDown = 1u << 7,
    PadL1 = 1u << 8,
    PadR1 = 1u << 9,
    PadL3 = 1u << 10,
    PadR3 = 1u << 11,
    PadOptions = 1u << 12,
    PadShare = 1u << 13,
    PadTouchpad = 1u << 14,
    PadPs = 1u << 15,
};

// One controller frame. Sticks: -32768..32767, Y positive = down (same as SDL/DualSense).
// Triggers: 0..255.
struct PadState {
    std::uint32_t buttons = 0;
    std::uint8_t l2 = 0;
    std::uint8_t r2 = 0;
    std::int16_t lx = 0;
    std::int16_t ly = 0;
    std::int16_t rx = 0;
    std::int16_t ry = 0;

    bool operator==(const PadState& other) const noexcept;
    bool operator!=(const PadState& other) const noexcept { return !(*this == other); }
    bool IsNeutral() const noexcept { return *this == PadState{}; }
};

// Full-scale stick deflection in [-1, 1] -> int16 stick value, clamped.
std::int16_t StickFromUnit(float unit) noexcept;

// Trigger value in [0, 1] -> 0..255.
std::uint8_t TriggerFromUnit(float unit) noexcept;

}  // namespace pskb
