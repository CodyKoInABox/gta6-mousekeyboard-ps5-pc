#include <cmath>
#include <limits>

#include "core/pad_state.h"
#include "test_framework.h"

using namespace pskb;

PSKB_TEST(StickFromUnitMapsFullScale) {
    CHECK_EQ(StickFromUnit(1.0f), static_cast<std::int16_t>(32767));
    CHECK_EQ(StickFromUnit(-1.0f), static_cast<std::int16_t>(-32767));
    CHECK_EQ(StickFromUnit(0.0f), static_cast<std::int16_t>(0));
}

PSKB_TEST(StickFromUnitClampsOutOfRange) {
    CHECK_EQ(StickFromUnit(3.0f), static_cast<std::int16_t>(32767));
    CHECK_EQ(StickFromUnit(-7.5f), static_cast<std::int16_t>(-32767));
}

PSKB_TEST(StickFromUnitRejectsNaN) {
    CHECK_EQ(StickFromUnit(std::numeric_limits<float>::quiet_NaN()), static_cast<std::int16_t>(0));
}

PSKB_TEST(StickFromUnitRoundsHalfWay) {
    const std::int16_t half = StickFromUnit(0.5f);
    CHECK(half >= 16383 && half <= 16384);
}

PSKB_TEST(TriggerFromUnitRange) {
    CHECK_EQ(TriggerFromUnit(0.0f), static_cast<std::uint8_t>(0));
    CHECK_EQ(TriggerFromUnit(1.0f), static_cast<std::uint8_t>(255));
    CHECK_EQ(TriggerFromUnit(2.0f), static_cast<std::uint8_t>(255));
    CHECK_EQ(TriggerFromUnit(-1.0f), static_cast<std::uint8_t>(0));
    CHECK_EQ(TriggerFromUnit(std::numeric_limits<float>::quiet_NaN()), static_cast<std::uint8_t>(0));
}

PSKB_TEST(PadStateEqualityAndNeutral) {
    PadState a;
    CHECK(a.IsNeutral());
    PadState b;
    b.buttons = PadCross;
    CHECK(a != b);
    CHECK(!b.IsNeutral());
    b.buttons = 0;
    CHECK(a == b);
}

PSKB_TEST(PadStateEqualityConsidersSticksAndTriggers) {
    PadState a;
    PadState b;
    b.rx = 1;
    CHECK(a != b);
    b = a;
    b.r2 = 1;
    CHECK(a != b);
}
