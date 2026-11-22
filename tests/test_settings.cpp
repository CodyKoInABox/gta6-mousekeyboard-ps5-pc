#include <filesystem>

#include "core/settings.h"
#include "test_framework.h"

using namespace pskb;

PSKB_TEST(SettingsRoundTripKeepsEveryField) {
    AppSettings s;
    s.language = "pt-BR";
    s.darkTheme = true;
    s.autoConnectOnLaunch = true;
    s.autoReconnect = false;
    s.minimizeToTray = false;
    s.launchVeyra = false;
    s.veyraPath = "C:\\Program Files\\Veyra\\Veyra.exe";
    s.profileId = "driving";
    s.captureHotkeyVk = 0x78;
    s.aim.sensitivityX = 1.25f;
    s.aim.sensitivityY = 0.5f;
    s.aim.invertY = true;
    s.aim.deadzone = 0.05f;
    s.aim.antiDeadzone = 0.2f;
    s.aim.exponentialCurve = true;
    s.aim.curveExponent = 2.2f;
    s.aim.acceleration = 0.4f;
    s.aim.smoothingMs = 12.0f;
    s.aim.recenterMs = 120.0f;
    s.aim.windowMs = 20.0f;
    s.aim.fullDeflectionCountsPerSec = 3000.0f;

    AppSettings back;
    CHECK(ParseSettings(SerializeSettings(s), back));
    CHECK_EQ(back.language, s.language);
    CHECK(back.autoConnectOnLaunch);
    CHECK(!back.autoReconnect);
    CHECK(!back.minimizeToTray);
    CHECK(!back.launchVeyra);
    CHECK_EQ(back.veyraPath, s.veyraPath);
    CHECK_EQ(back.profileId, s.profileId);
    CHECK_EQ(back.captureHotkeyVk, s.captureHotkeyVk);
    CHECK_NEAR(back.aim.sensitivityX, 1.25, 1e-4);
    CHECK_NEAR(back.aim.sensitivityY, 0.5, 1e-4);
    CHECK(back.aim.invertY);
    CHECK_NEAR(back.aim.deadzone, 0.05, 1e-4);
    CHECK_NEAR(back.aim.antiDeadzone, 0.2, 1e-4);
    CHECK(back.aim.exponentialCurve);
    CHECK_NEAR(back.aim.curveExponent, 2.2, 1e-4);
    CHECK_NEAR(back.aim.acceleration, 0.4, 1e-4);
    CHECK_NEAR(back.aim.smoothingMs, 12.0, 1e-4);
    CHECK_NEAR(back.aim.recenterMs, 120.0, 1e-4);
    CHECK_NEAR(back.aim.windowMs, 20.0, 1e-4);
    CHECK_NEAR(back.aim.fullDeflectionCountsPerSec, 3000.0, 1e-2);
}

PSKB_TEST(SettingsIgnoreUnknownKeysForForwardCompatibility) {
    AppSettings s;
    CHECK(ParseSettings("future_feature=yes\nlanguage=pt-BR\n", s));
    CHECK_EQ(s.language, std::string("pt-BR"));
}

PSKB_TEST(SettingsKeepDefaultsForMissingKeys) {
    AppSettings s;
    s.profileId = "gta6";
    CHECK(ParseSettings("minimize_to_tray=false\n", s));
    CHECK(!s.minimizeToTray);
    CHECK_EQ(s.profileId, std::string("gta6"));
    CHECK(s.autoReconnect);
}

PSKB_TEST(SettingsRejectInvalidValuesWithoutChangingState) {
    AppSettings s;
    s.minimizeToTray = true;
    CHECK(!ParseSettings("minimize_to_tray=maybe\n", s));
    CHECK(s.minimizeToTray);

    CHECK(!ParseSettings("aim.sensitivity_x=fast\n", s));
    CHECK_NEAR(s.aim.sensitivityX, 1.0, 1e-6);

    CHECK(!ParseSettings("capture_hotkey_vk=-5\n", s));
    CHECK(!ParseSettings("this line has no equals sign\n", s));
}

PSKB_TEST(SettingsFileSaveAndLoad) {
    const auto dir = std::filesystem::temp_directory_path() / L"pskb-tests" / L"settings";
    std::filesystem::remove_all(dir);
    const auto path = dir / "settings.ini";

    AppSettings s;
    s.profileId = "driving";
    s.veyraPath = "D:\\Veyra\\Veyra.exe";
    CHECK(SaveSettingsFile(path, s));

    AppSettings loaded;
    CHECK(LoadSettingsFile(path, loaded));
    CHECK_EQ(loaded.profileId, std::string("driving"));
    CHECK_EQ(loaded.veyraPath, std::string("D:\\Veyra\\Veyra.exe"));

    AppSettings missing;
    CHECK(!LoadSettingsFile(dir / "nope.ini", missing));
}
