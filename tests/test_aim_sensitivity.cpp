#include <cmath>

#include "core/mouse_to_stick.h"
#include "core/pad_engine.h"
#include "test_framework.h"

using namespace pskb;

namespace {

AimSettings LinearAim() {
    AimSettings s;
    s.deadzone = 0.0f;
    s.antiDeadzone = 0.0f;
    s.acceleration = 0.0f;
    s.smoothingMs = 0.0f;
    s.exponentialCurve = false;
    s.invertY = false;
    s.fullDeflectionCountsPerSec = 1000.0f;
    s.windowMs = 10.0f;
    s.recenterMs = 50.0f;
    return s;
}

}  // namespace

PSKB_TEST(AimingUsesAimSensitivityAndCameraUsesCameraSensitivity) {
    AimSettings s = LinearAim();
    s.sensitivityX = 0.2f;
    s.aimSensitivityX = 0.1f;
    MouseToStick camera(s);
    MouseToStick aim(s);
    for (int i = 0; i < 10; ++i) {
        camera.AddMotion(1.0 * i, 1, 0);
        aim.AddMotion(1.0 * i, 1, 0);
    }
    const float cam = camera.Evaluate(10.0, false).x;
    const float aimed = aim.Evaluate(10.0, true).x;
    CHECK(cam > 0.0f);
    CHECK(cam < 1.0f);
    CHECK_NEAR(aimed * 2.0f, cam, 1e-4);
}

PSKB_TEST(AimEvaluateDefaultsToCameraSensitivity) {
    AimSettings s = LinearAim();
    s.sensitivityX = 0.2f;
    s.aimSensitivityX = 0.1f;
    MouseToStick a(s);
    MouseToStick b(s);
    for (int i = 0; i < 10; ++i) {
        a.AddMotion(1.0 * i, 1, 0);
        b.AddMotion(1.0 * i, 1, 0);
    }
    CHECK_NEAR(a.Evaluate(10.0).x, b.Evaluate(10.0, false).x, 1e-6);
}

PSKB_TEST(AimSensitivityIsClampedLikeOtherScales) {
    AimSettings s = LinearAim();
    s.aimSensitivityX = 1000.0f;
    s.aimSensitivityY = 0.0f;
    MouseToStick m(s);
    CHECK(m.Settings().aimSensitivityX <= 10.0f);
    CHECK(m.Settings().aimSensitivityY >= 0.05f);
}

PSKB_TEST(PadEngineAimsWithAimSensitivityOnlyWhileL2IsHeld) {
    Profile profile = GtaProfile();
    profile.aim = LinearAim();
    profile.aim.sensitivityX = 0.4f;
    profile.aim.sensitivityY = 0.4f;
    profile.aim.aimSensitivityX = 0.1f;
    profile.aim.aimSensitivityY = 0.1f;
    profile.aim.fullDeflectionCountsPerSec = 2000.0f;
    profile.stickRampMs = 0.0f;
    profile.triggerRampMs = 0.0f;

    PadEngine camera(profile);
    PadEngine aiming(profile);
    aiming.KeyDown(kMouseRight);
    for (int i = 0; i < 10; ++i) {
        camera.MouseMove(1, 0, 1.0 * i);
        aiming.MouseMove(1, 0, 1.0 * i);
    }
    const PadState cameraPad = camera.Update(10.0);
    const PadState aimPad = aiming.Update(10.0);
    CHECK(cameraPad.rx > 0);
    CHECK(aimPad.rx > 0);
    CHECK(aimPad.rx * 4 <= cameraPad.rx + 8);
    CHECK(aimPad.rx * 4 >= cameraPad.rx - 8);
}
