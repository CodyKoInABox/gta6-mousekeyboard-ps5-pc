#include <cmath>
#include <string>

#include "core/bindings.h"
#include "core/settings.h"
#include "test_framework.h"

using namespace pskb;

PSKB_TEST(SettingsRoundTripKeepsAimSensitivityCurveAndSmoothing) {
    AppSettings in;
    in.aim.sensitivityX = 1.7f;
    in.aim.sensitivityY = 0.9f;
    in.aim.aimSensitivityX = 0.35f;
    in.aim.aimSensitivityY = 0.6f;
    in.aim.exponentialCurve = true;
    in.aim.curveExponent = 2.2f;
    in.aim.smoothingMs = 45.0f;

    AppSettings out;
    CHECK(ParseSettings(SerializeSettings(in), out));
    CHECK_NEAR(out.aim.sensitivityX, 1.7f, 1e-4f);
    CHECK_NEAR(out.aim.sensitivityY, 0.9f, 1e-4f);
    CHECK_NEAR(out.aim.aimSensitivityX, 0.35f, 1e-4f);
    CHECK_NEAR(out.aim.aimSensitivityY, 0.6f, 1e-4f);
    CHECK(out.aim.exponentialCurve);
    CHECK_NEAR(out.aim.curveExponent, 2.2f, 1e-4f);
    CHECK_NEAR(out.aim.smoothingMs, 45.0f, 1e-4f);
}

PSKB_TEST(SettingsRoundTripKeepsSavedBindingsPerProfile) {
    AppSettings in;
    in.savedBindings["gta6"] = "F=cross;W=left_stick_up";
    in.savedBindings["driving"] = "W=r2;S=l2";

    AppSettings out;
    CHECK(ParseSettings(SerializeSettings(in), out));
    CHECK_EQ(out.savedBindings.size(), std::size_t{2});
    CHECK_EQ(out.savedBindings["gta6"], std::string("F=cross;W=left_stick_up"));
    CHECK_EQ(out.savedBindings["driving"], std::string("W=r2;S=l2"));
}

PSKB_TEST(SavedBindingsTextSurvivesSettingsFileAndParsesBack) {
    Profile edited = GtaProfile();
    CHECK(!AddBinding(edited, Action::Ps, 0x2D).has_value());

    AppSettings in;
    in.savedBindings["gta6"] = SerializeBindingsInline(edited);

    AppSettings out;
    CHECK(ParseSettings(SerializeSettings(in), out));
    const Profile restored = ProfileWithBindings("gta6", out.savedBindings["gta6"]);
    CHECK_EQ(SerializeBindingsInline(restored), SerializeBindingsInline(edited));
}

PSKB_TEST(OlderSettingsFilesLoadWithDefaultAimAndNoSavedBindings) {
    const std::string legacy =
        "profile=driving\n"
        "aim.sensitivity_x=1.2\n"
        "aim.deadzone=0.05\n";
    AppSettings out;
    CHECK(ParseSettings(legacy, out));
    CHECK_EQ(out.profileId, std::string("driving"));
    CHECK_NEAR(out.aim.sensitivityX, 1.2f, 1e-4f);
    CHECK(out.savedBindings.empty());
    CHECK_NEAR(out.aim.aimSensitivityX, AimSettings{}.aimSensitivityX, 1e-6f);
    CHECK_NEAR(out.aim.smoothingMs, 0.0f, 1e-6f);
}

PSKB_TEST(InvalidAimValueInSettingsFileIsRejected) {
    AppSettings out;
    CHECK(!ParseSettings("aim.aim_sensitivity_x=fast\n", out));
}

PSKB_TEST(EmptyBindingsEntryFallsBackToDefaultsAndNamelessKeyIsIgnored) {
    AppSettings out;
    CHECK(ParseSettings("bindings.=W=cross\nbindings.gta6=\n", out));
    CHECK_EQ(out.savedBindings.size(), std::size_t{1});
    CHECK(out.savedBindings.count("gta6") == 1);
    CHECK(ProfileWithBindings("gta6", out.savedBindings["gta6"]).bindings.size() == GtaProfile().bindings.size());
}
