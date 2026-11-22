#include <string>

#include "core/bindings.h"
#include "core/capture_session.h"
#include "core/pad_engine.h"
#include "test_framework.h"

using namespace pskb;

namespace {

constexpr InputCode kHotkeyF8 = 0x77;
constexpr InputCode kSpace = 0x20;
constexpr InputCode kW = 'W';

std::string Mode(const CaptureSession& s) { return CaptureModeName(s.Mode()); }

}  // namespace

PSKB_TEST(CaptureOffIgnoresAllInput) {
    PadEngine engine(GtaProfile());
    CaptureSession session(engine, kHotkeyF8);
    session.SetGameFocused(true);
    session.SetLinkReady(true);
    session.OnKey(kSpace, true);
    session.OnMouseButton(kMouseLeft, true);
    session.OnMouseMove(500, 0, 0.0);
    CHECK_EQ(Mode(session), std::string("off"));
    CHECK(session.Tick(0.0).IsNeutral());
    CHECK_EQ(engine.HeldCount(), static_cast<std::size_t>(0));
}

PSKB_TEST(ArmedWithoutFocusDoesNotForward) {
    PadEngine engine(GtaProfile());
    CaptureSession session(engine, kHotkeyF8);
    session.SetLinkReady(true);
    session.Toggle();
    CHECK_EQ(Mode(session), std::string("armed"));
    session.OnKey(kSpace, true);
    CHECK(session.Tick(0.0).IsNeutral());
    CHECK_EQ(engine.HeldCount(), static_cast<std::size_t>(0));
}

PSKB_TEST(ArmedWithoutLinkDoesNotForward) {
    PadEngine engine(GtaProfile());
    CaptureSession session(engine, kHotkeyF8);
    session.SetGameFocused(true);
    session.Toggle();
    CHECK_EQ(Mode(session), std::string("armed"));
    session.OnKey(kSpace, true);
    CHECK(session.Tick(0.0).IsNeutral());
}

PSKB_TEST(ActiveWhenFocusedAndLinked) {
    PadEngine engine(GtaProfile());
    CaptureSession session(engine, kHotkeyF8);
    session.Toggle();
    session.SetLinkReady(true);
    session.SetGameFocused(true);
    CHECK_EQ(Mode(session), std::string("active"));
    session.OnKey(kSpace, true);
    CHECK((session.Tick(0.0).buttons & PadCross) != 0);
}

PSKB_TEST(FocusLossReleasesEveryInputAndRecentres) {
    PadEngine engine(GtaProfile());
    CaptureSession session(engine, kHotkeyF8);
    session.Toggle();
    session.SetLinkReady(true);
    session.SetGameFocused(true);
    session.OnKey(kSpace, true);
    session.OnKey(kW, true);
    session.OnMouseButton(kMouseLeft, true);
    session.OnMouseMove(300, 0, 0.0);
    session.Tick(0.0);
    session.Tick(100.0);

    session.SetGameFocused(false);
    CHECK_EQ(Mode(session), std::string("armed"));
    CHECK_EQ(engine.HeldCount(), static_cast<std::size_t>(0));
    CHECK(session.Tick(200.0).IsNeutral());
}

PSKB_TEST(LinkLossReleasesEveryInput) {
    PadEngine engine(GtaProfile());
    CaptureSession session(engine, kHotkeyF8);
    session.Toggle();
    session.SetLinkReady(true);
    session.SetGameFocused(true);
    session.OnKey(kSpace, true);
    session.SetLinkReady(false);
    CHECK_EQ(Mode(session), std::string("armed"));
    CHECK(session.Tick(0.0).IsNeutral());
}

PSKB_TEST(KeyUpIsAlwaysForwardedSoNothingSticks) {
    PadEngine engine(GtaProfile());
    CaptureSession session(engine, kHotkeyF8);
    session.Toggle();
    session.SetLinkReady(true);
    session.SetGameFocused(true);
    session.OnKey(kSpace, true);
    session.SetGameFocused(false);
    session.OnKey(kSpace, false);
    session.SetGameFocused(true);
    CHECK(!engine.IsHeld(kSpace));
    CHECK(session.Tick(0.0).IsNeutral());
}

PSKB_TEST(KeyPressedWhileNotActiveIsNotTracked) {
    PadEngine engine(GtaProfile());
    CaptureSession session(engine, kHotkeyF8);
    session.Toggle();
    session.OnKey(kSpace, true);
    CHECK(!engine.IsHeld(kSpace));
    session.SetLinkReady(true);
    session.SetGameFocused(true);
    CHECK(session.Tick(0.0).IsNeutral());
}

PSKB_TEST(EscapeDisarmsAndReleasesWhileActive) {
    PadEngine engine(GtaProfile());
    CaptureSession session(engine, kHotkeyF8);
    session.Toggle();
    session.SetLinkReady(true);
    session.SetGameFocused(true);
    session.OnKey(kSpace, true);
    session.OnKey(CaptureSession::kEscape, true);
    CHECK_EQ(Mode(session), std::string("off"));
    CHECK_EQ(engine.HeldCount(), static_cast<std::size_t>(0));
    session.OnKey(kSpace, true);
    CHECK(session.Tick(0.0).IsNeutral());
}

PSKB_TEST(EscapeIsNotForwardedAsAGameKey) {
    PadEngine engine(GtaProfile());
    CaptureSession session(engine, kHotkeyF8);
    session.Toggle();
    session.SetLinkReady(true);
    session.SetGameFocused(true);
    session.OnKey(CaptureSession::kEscape, true);
    CHECK_EQ(Mode(session), std::string("off"));
    CHECK(!engine.IsHeld(CaptureSession::kEscape));
}

PSKB_TEST(EscapeWithCaptureOffDoesNothing) {
    PadEngine engine(GtaProfile());
    CaptureSession session(engine, kHotkeyF8);
    session.SetLinkReady(true);
    session.SetGameFocused(true);
    session.OnKey(CaptureSession::kEscape, true);
    CHECK_EQ(Mode(session), std::string("off"));
    CHECK(!engine.IsHeld(CaptureSession::kEscape));
}

PSKB_TEST(HotkeyTogglesOncePerPressAndIsNotForwarded) {
    PadEngine engine(GtaProfile());
    CaptureSession session(engine, kHotkeyF8);
    session.OnKey(kHotkeyF8, true);
    CHECK_EQ(Mode(session), std::string("armed"));
    session.OnKey(kHotkeyF8, true);
    CHECK_EQ(Mode(session), std::string("armed"));
    session.OnKey(kHotkeyF8, false);
    CHECK_EQ(Mode(session), std::string("armed"));
    session.OnKey(kHotkeyF8, true);
    CHECK_EQ(Mode(session), std::string("off"));
    CHECK(!engine.IsHeld(kHotkeyF8));
}

PSKB_TEST(ToggleOffReleasesWhileActive) {
    PadEngine engine(GtaProfile());
    CaptureSession session(engine, kHotkeyF8);
    session.Toggle();
    session.SetLinkReady(true);
    session.SetGameFocused(true);
    session.OnKey(kSpace, true);
    session.Toggle();
    CHECK_EQ(Mode(session), std::string("off"));
    CHECK_EQ(engine.HeldCount(), static_cast<std::size_t>(0));
}

PSKB_TEST(MouseMotionIgnoredUntilActive) {
    PadEngine engine(GtaProfile());
    CaptureSession session(engine, kHotkeyF8);
    session.SetLinkReady(true);
    session.Toggle();
    session.OnMouseMove(500, 0, 0.0);
    session.SetGameFocused(true);
    const PadState s = session.Tick(0.0);
    CHECK_EQ(static_cast<int>(s.rx), 0);
}

PSKB_TEST(MouseButtonDrivesTriggerWhileActive) {
    PadEngine engine(GtaProfile());
    CaptureSession session(engine, kHotkeyF8);
    session.Toggle();
    session.SetLinkReady(true);
    session.SetGameFocused(true);
    session.OnMouseButton(kMouseLeft, true);
    CHECK_EQ(static_cast<int>(session.Tick(1000.0).r2), 255);
    session.OnMouseButton(kMouseLeft, false);
    CHECK_EQ(static_cast<int>(session.Tick(2000.0).r2), 0);
}

PSKB_TEST(CustomHotkeyReplacesDefault) {
    PadEngine engine(GtaProfile());
    CaptureSession session(engine, kHotkeyF8);
    session.SetHotkey(0x78);  // F9
    session.OnKey(kHotkeyF8, true);
    CHECK_EQ(Mode(session), std::string("off"));
    session.OnKey(0x78, true);
    CHECK_EQ(Mode(session), std::string("armed"));
}
