#include <cmath>

#include "core/pad_engine.h"
#include "test_framework.h"

using namespace pskb;

namespace {

constexpr InputCode kW = 'W';
constexpr InputCode kA = 'A';
constexpr InputCode kD = 'D';
constexpr InputCode kSpace = 0x20;

Profile TinyProfile() {
    Profile p;
    p.id = "test";
    p.bindings = {
        {kW, Action::LeftStickUp},
        {kA, Action::LeftStickLeft},
        {kD, Action::LeftStickRight},
        {'S', Action::LeftStickDown},
        {kSpace, Action::Cross},
        {'J', Action::Cross},
        {kMouseLeft, Action::R2},
    };
    p.aim.fullDeflectionCountsPerSec = 1000.0f;
    p.aim.recenterMs = 50.0f;
    p.aim.windowMs = 10.0f;
    return p;
}

}  // namespace

PSKB_TEST(KeyDownPressesButtonOnNextFrame) {
    PadEngine e(TinyProfile());
    e.KeyDown(kSpace);
    const PadState s = e.Update(0.0);
    CHECK((s.buttons & PadCross) != 0);
}

PSKB_TEST(KeyUpReleasesButtonOnNextFrame) {
    PadEngine e(TinyProfile());
    e.KeyDown(kSpace);
    e.Update(0.0);
    e.KeyUp(kSpace);
    const PadState s = e.Update(8.0);
    CHECK((s.buttons & PadCross) == 0);
    CHECK(s.IsNeutral());
}

PSKB_TEST(RepeatedKeyDownDoesNotDoubleCount) {
    PadEngine e(TinyProfile());
    e.KeyDown(kSpace);
    e.KeyDown(kSpace);
    CHECK_EQ(e.HeldCount(), static_cast<std::size_t>(1));
    e.KeyUp(kSpace);
    CHECK(!e.IsHeld(kSpace));
}

PSKB_TEST(TwoInputsForOneActionStayPressedUntilBothRelease) {
    PadEngine e(TinyProfile());
    e.KeyDown(kSpace);
    e.KeyDown('J');
    e.KeyUp(kSpace);
    CHECK((e.Update(0.0).buttons & PadCross) != 0);
    e.KeyUp('J');
    CHECK((e.Update(8.0).buttons & PadCross) == 0);
}

PSKB_TEST(ReleaseAllClearsEverythingImmediately) {
    PadEngine e(TinyProfile());
    e.KeyDown(kW);
    e.KeyDown(kD);
    e.KeyDown(kSpace);
    e.MouseMove(10, 0, 0.0);
    e.Update(0.0);
    e.ReleaseAll();
    CHECK_EQ(e.HeldCount(), static_cast<std::size_t>(0));
    const PadState s = e.Update(8.0);
    CHECK(s.IsNeutral());
}

PSKB_TEST(WasdDiagonalIsNormalised) {
    PadEngine e(TinyProfile());
    e.KeyDown(kW);
    e.KeyDown(kD);
    const PadState s = e.Update(0.0);
    const double len = std::sqrt(static_cast<double>(s.lx) * s.lx + static_cast<double>(s.ly) * s.ly);
    CHECK_NEAR(len, 32767.0, 40.0);
    CHECK(s.lx > 0);
    CHECK(s.ly < 0);  // W is up: negative Y
}

PSKB_TEST(WasdOpposingKeysCancel) {
    PadEngine e(TinyProfile());
    e.KeyDown(kA);
    e.KeyDown(kD);
    const PadState s = e.Update(0.0);
    CHECK_EQ(s.lx, static_cast<std::int16_t>(0));
}

PSKB_TEST(SStickDownIsPositiveY) {
    PadEngine e(TinyProfile());
    e.KeyDown('S');
    CHECK(e.Update(0.0).ly > 0);
}

PSKB_TEST(MouseLeftPullsR2Fully) {
    PadEngine e(TinyProfile());
    e.KeyDown(kMouseLeft);
    CHECK_EQ(e.Update(0.0).r2, static_cast<std::uint8_t>(0xFF));
    e.KeyUp(kMouseLeft);
    CHECK_EQ(e.Update(8.0).r2, static_cast<std::uint8_t>(0));
}

PSKB_TEST(MouseMotionDrivesRightStickAndRecentres) {
    PadEngine e(TinyProfile());
    for (int i = 0; i < 8; ++i) e.MouseMove(6, 0, static_cast<double>(i));
    const PadState moving = e.Update(8.0);
    CHECK(moving.rx > 10000);
    CHECK_EQ(moving.ry, static_cast<std::int16_t>(0));
    const PadState still = e.Update(200.0);
    CHECK_EQ(still.rx, static_cast<std::int16_t>(0));
}

PSKB_TEST(MouseMotionNeverTouchesLeftStickOrButtons) {
    PadEngine e(TinyProfile());
    for (int i = 0; i < 8; ++i) e.MouseMove(20, 20, static_cast<double>(i));
    const PadState s = e.Update(8.0);
    CHECK_EQ(s.lx, static_cast<std::int16_t>(0));
    CHECK_EQ(s.ly, static_cast<std::int16_t>(0));
    CHECK_EQ(s.buttons, 0u);
}

PSKB_TEST(SetProfileReleasesHeldInputs) {
    PadEngine e(TinyProfile());
    e.KeyDown(kSpace);
    e.SetProfile(TinyProfile());
    CHECK_EQ(e.HeldCount(), static_cast<std::size_t>(0));
    CHECK(e.Update(0.0).IsNeutral());
}

PSKB_TEST(SteeringRampReachesFullOverConfiguredTime) {
    Profile p = TinyProfile();
    p.stickRampMs = 200.0f;
    PadEngine e(p);
    e.KeyDown(kD);
    e.Update(0.0);
    const auto half = e.Update(100.0).lx;
    CHECK(half > 14000 && half < 19000);
    const auto full = e.Update(200.0).lx;
    CHECK(full > 32000);
}

PSKB_TEST(SteeringRampReleasesGradually) {
    Profile p = TinyProfile();
    p.stickRampMs = 200.0f;
    PadEngine e(p);
    e.KeyDown(kD);
    e.Update(0.0);
    e.Update(400.0);
    e.KeyUp(kD);
    const auto afterSmallStep = e.Update(450.0).lx;
    CHECK(afterSmallStep > 0);
    CHECK(afterSmallStep < 32000);
}

PSKB_TEST(TriggerRampIsGradual) {
    Profile p = TinyProfile();
    p.triggerRampMs = 100.0f;
    PadEngine e(p);
    e.KeyDown(kMouseLeft);
    e.Update(0.0);
    const auto mid = e.Update(50.0).r2;
    CHECK(mid > 100 && mid < 160);
    CHECK_EQ(e.Update(200.0).r2, static_cast<std::uint8_t>(0xFF));
}

PSKB_TEST(LongFrameGapsAreClampedInRamp) {
    Profile p = TinyProfile();
    p.stickRampMs = 200.0f;
    PadEngine e(p);
    e.KeyDown(kD);
    e.Update(0.0);
    // A 10-second stall counts as one 200 ms step, so the ramp finishes but never overshoots.
    const auto after = e.Update(10000.0).lx;
    CHECK(after <= 32767);
    CHECK(after >= 32000);
}
