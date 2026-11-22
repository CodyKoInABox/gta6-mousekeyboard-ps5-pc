#include "core/raw_keys.h"

namespace pskb {
namespace {

constexpr std::uint16_t kVkShift = 0x10;
constexpr std::uint16_t kVkControl = 0x11;
constexpr std::uint16_t kVkMenu = 0x12;
constexpr std::uint16_t kVkLShift = 0xA0;
constexpr std::uint16_t kVkRShift = 0xA1;
constexpr std::uint16_t kVkLControl = 0xA2;
constexpr std::uint16_t kVkRControl = 0xA3;
constexpr std::uint16_t kVkLMenu = 0xA4;
constexpr std::uint16_t kVkRMenu = 0xA5;
constexpr std::uint16_t kScanRShift = 0x36;

}  // namespace

InputCode KeyCodeFromRaw(std::uint16_t virtualKey, std::uint16_t makeCode, bool extended) noexcept {
    switch (virtualKey) {
        case kVkShift:
            if (makeCode == kScanRShift) return kVkRShift;
            return kVkLShift;
        case kVkControl:
            return extended ? kVkRControl : kVkLControl;
        case kVkMenu:
            return extended ? kVkRMenu : kVkLMenu;
        default:
            return virtualKey;
    }
}

}  // namespace pskb
