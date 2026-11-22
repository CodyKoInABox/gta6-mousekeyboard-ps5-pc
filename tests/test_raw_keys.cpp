#include "core/raw_keys.h"
#include "test_framework.h"

using namespace pskb;

PSKB_TEST(ShiftKeysResolveToLeftAndRight) {
    CHECK_EQ(static_cast<int>(KeyCodeFromRaw(0x10, 0x2A, false)), 0xA0);
    CHECK_EQ(static_cast<int>(KeyCodeFromRaw(0x10, 0x36, false)), 0xA1);
}

PSKB_TEST(ControlKeysResolveToLeftAndRight) {
    CHECK_EQ(static_cast<int>(KeyCodeFromRaw(0x11, 0x1D, false)), 0xA2);
    CHECK_EQ(static_cast<int>(KeyCodeFromRaw(0x11, 0x1D, true)), 0xA3);
}

PSKB_TEST(AltKeysResolveToLeftAndRight) {
    CHECK_EQ(static_cast<int>(KeyCodeFromRaw(0x12, 0x38, false)), 0xA4);
    CHECK_EQ(static_cast<int>(KeyCodeFromRaw(0x12, 0x38, true)), 0xA5);
}

PSKB_TEST(OtherKeysPassThroughUnchanged) {
    CHECK_EQ(static_cast<int>(KeyCodeFromRaw(0x20, 0x39, false)), 0x20);
    CHECK_EQ(static_cast<int>(KeyCodeFromRaw('W', 0x11, false)), static_cast<int>('W'));
    CHECK_EQ(static_cast<int>(KeyCodeFromRaw(0x25, 0x4B, true)), 0x25);
}
