#include <cmath>

#include "core/mouse_to_stick.h"
#include "test_framework.h"

using namespace pskb;

namespace {

AimSettings Linear() {
    AimSettings s;
    s.deadzone = 0.0f;
    s.antiDeadzone = 0.0f;
    s.acceleration = 0.0f;
    s.smoothingMs = 0.0f;
    s.sensitivityX = 1.0f;
    s.sensitivityY = 1.0f;
    s.fullDeflectionCountsPerSec = 1000.0f;
    s.windowMs = 10.0f;
    s.recenterMs = 50.0f;
    return s;
}

}  // namespace

PSKB_TEST(MouseNoMotionIsNeutral) {
    MouseToStick m(Linear());
    const StickVector v = m.Evaluate(100.0);
    CHECK_NEAR(v.x, 0.0, 1e-6);
    CHECK_NEAR(v.y, 0.0, 1e-6);
}

PSKB_TEST(MouseRightPushesStickRight) {
    MouseToStick m(Linear());
    for (int i = 0; i < 10; ++i) m.AddMotion(1.0 * i, 5, 0);
    const StickVector v = m.Evaluate(10.0);
    CHECK(v.x > 0.5f);
    CHECK_NEAR(v.y, 0.0, 1e-6);
}

PSKB_TEST(MouseDownPushesStickDownPositiveY) {
    MouseToStick m(Linear());
    for (int i = 0; i < 10; ++i) m.AddMotion(1.0 * i, 0, 5);
    const StickVector v = m.Evaluate(10.0);
    CHECK(v.y > 0.5f);
}

PSKB_TEST(InvertYFlipsVertical) {
    AimSettings s = Linear();
    s.invertY = true;
    MouseToStick m(s);
    for (int i = 0; i < 10; ++i) m.AddMotion(1.0 * i, 0, 5);
    CHECK(m.Evaluate(10.0).y < -0.5f);
}

PSKB_TEST(VerticalSensitivityScalesOnlyY) {
    AimSettings s = Linear();
    s.sensitivityY = 0.25f;
    MouseToStick m(s);
    for (int i = 0; i < 10; ++i) m.AddMotion(1.0 * i, 4, 4);
    const StickVector v = m.Evaluate(10.0);
    CHECK(std::fabs(v.x) > std::fabs(v.y) * 2.0f);
}

PSKB_TEST(StickReturnsToCentreAfterRecentre) {
    MouseToStick m(Linear());
    for (int i = 0; i < 10; ++i) m.AddMotion(1.0 * i, 5, 0);
    CHECK(m.Evaluate(10.0).x > 0.0f);
    const StickVector v = m.Evaluate(10.0 + 60.0);
    CHECK_NEAR(v.x, 0.0, 1e-6);
    CHECK_NEAR(v.y, 0.0, 1e-6);
}

PSKB_TEST(OutputNeverExceedsUnitCircle) {
    AimSettings s = Linear();
    s.sensitivityX = 10.0f;
    s.sensitivityY = 10.0f;
    MouseToStick m(s);
    for (int i = 0; i < 10; ++i) m.AddMotion(1.0 * i, 500, 500);
    const StickVector v = m.Evaluate(10.0);
    const float len = std::sqrt(v.x * v.x + v.y * v.y);
    CHECK(len <= 1.0001f);
    CHECK(len > 0.99f);
}

PSKB_TEST(OutputIsIndependentOfEvaluationRate) {
    // Same mouse stream, sampled at 60 Hz and at 240 Hz, must agree at shared timestamps.
    AimSettings s = Linear();
    s.acceleration = 0.3f;
    MouseToStick coarse(s);
    MouseToStick fine(s);
    for (int ms = 0; ms < 200; ++ms) {
        if (ms % 2 == 0) {
            coarse.AddMotion(ms, 3, -2);
            fine.AddMotion(ms, 3, -2);
        }
        if (ms % 4 == 0) fine.Evaluate(ms);
        if (ms % 16 == 0) coarse.Evaluate(ms);
    }
    const StickVector a = coarse.Evaluate(192.0);
    const StickVector b = fine.Evaluate(192.0);
    CHECK_NEAR(a.x, b.x, 1e-5);
    CHECK_NEAR(a.y, b.y, 1e-5);
}

PSKB_TEST(DeadzoneIgnoresTinyMotion) {
    AimSettings s = Linear();
    s.deadzone = 0.2f;
    s.fullDeflectionCountsPerSec = 10000.0f;
    MouseToStick m(s);
    m.AddMotion(0.0, 1, 0);
    CHECK_NEAR(m.Evaluate(1.0).x, 0.0, 1e-6);
}

PSKB_TEST(AntiDeadzoneJumpsStartOfDeflection) {
    AimSettings s = Linear();
    s.antiDeadzone = 0.25f;
    CHECK_NEAR(MouseToStick::ShapeMagnitude(0.001f, s), 0.25f + 0.0f, 0.01f);
    CHECK_NEAR(MouseToStick::ShapeMagnitude(1.0f, s), 1.0f, 1e-6);
}

PSKB_TEST(LinearCurveIsIdentityWithoutDeadzones) {
    const AimSettings s = Linear();
    CHECK_NEAR(MouseToStick::ShapeMagnitude(0.3f, s), 0.3f, 1e-6);
    CHECK_NEAR(MouseToStick::ShapeMagnitude(0.7f, s), 0.7f, 1e-6);
}

PSKB_TEST(PowerCurveReducesMidRange) {
    AimSettings linear = Linear();
    AimSettings power = Linear();
    power.exponentialCurve = true;
    power.curveExponent = 2.0f;
    const float lin = MouseToStick::ShapeMagnitude(0.5f, linear);
    const float pow2 = MouseToStick::ShapeMagnitude(0.5f, power);
    CHECK(pow2 < lin);
    CHECK_NEAR(MouseToStick::ShapeMagnitude(1.0f, power), 1.0f, 1e-6);
}

PSKB_TEST(AccelerationRaisesFastMotion) {
    AimSettings slow = Linear();
    AimSettings fast = Linear();
    fast.acceleration = 1.0f;
    CHECK(MouseToStick::ShapeMagnitude(0.4f, fast) > MouseToStick::ShapeMagnitude(0.4f, slow));
}

PSKB_TEST(SmoothingLagsBehindTarget) {
    AimSettings s = Linear();
    s.smoothingMs = 50.0f;
    MouseToStick m(s);
    m.Evaluate(0.0);
    m.AddMotion(1.0, 10, 0);
    const float first = m.Evaluate(1.0).x;
    m.AddMotion(2.0, 10, 0);
    const float second = m.Evaluate(2.0).x;
    CHECK(first > 0.0f);
    CHECK(second > first);
    CHECK(second < 1.0f);
}

PSKB_TEST(ResetClearsMotion) {
    MouseToStick m(Linear());
    for (int i = 0; i < 10; ++i) m.AddMotion(1.0 * i, 5, 5);
    m.Reset();
    CHECK_NEAR(m.Evaluate(10.0).x, 0.0, 1e-6);
}

PSKB_TEST(ConfigureClampsOutOfRangeValues) {
    AimSettings s = Linear();
    s.sensitivityX = 1000.0f;
    s.deadzone = 0.9f;
    MouseToStick m(s);
    CHECK(m.Settings().sensitivityX <= 10.0f);
    CHECK(m.Settings().deadzone <= 0.5f);
}
