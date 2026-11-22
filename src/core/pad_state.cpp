#include "core/pad_state.h"

#include <algorithm>
#include <cmath>

namespace pskb {

bool PadState::operator==(const PadState& other) const noexcept {
    return buttons == other.buttons && l2 == other.l2 && r2 == other.r2 && lx == other.lx &&
           ly == other.ly && rx == other.rx && ry == other.ry;
}

std::int16_t StickFromUnit(float unit) noexcept {
    if (!(unit == unit)) return 0;  // NaN guard
    const float clamped = std::clamp(unit, -1.0f, 1.0f);
    return static_cast<std::int16_t>(std::lround(clamped * 32767.0f));
}

std::uint8_t TriggerFromUnit(float unit) noexcept {
    if (!(unit == unit)) return 0;
    const float clamped = std::clamp(unit, 0.0f, 1.0f);
    return static_cast<std::uint8_t>(std::lround(clamped * 255.0f));
}

}  // namespace pskb
