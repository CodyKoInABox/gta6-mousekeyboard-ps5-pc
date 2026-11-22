#include "core/input_gate.h"
#include "test_framework.h"

using namespace pskb;

namespace {

constexpr InputCode kSpace = 0x20;
constexpr InputCode kW = 'W';
constexpr InputCode kF8 = 0x77;
constexpr InputCode kTab = 0x09;
constexpr InputCode kDelete = 0x2E;
constexpr InputCode kLeftCtrl = 0xA2;
constexpr InputCode kLeftWinKey = 0x5B;

GateEvent Key(InputCode code, bool mapped = true) {
    GateEvent e;
    e.keyboard = true;
    e.down = true;
    e.code = code;
    e.mapped = mapped;
    return e;
}

GateEvent Button(InputCode code, bool down = true) {
    GateEvent e;
    e.keyboard = false;
    e.down = down;
    e.code = code;
    return e;
}

}  // namespace

PSKB_TEST(GateSwallowsMappedKeysOnlyWhenActiveAndFocused) {
    CHECK(ShouldSwallow(Key(kSpace), CaptureMode::Active, true));
    CHECK(ShouldSwallow(Key(kW), CaptureMode::Active, true));
}

PSKB_TEST(GatePassesEverythingOutsideActive) {
    CHECK(!ShouldSwallow(Key(kSpace), CaptureMode::Off, true));
    CHECK(!ShouldSwallow(Key(kSpace), CaptureMode::Armed, true));
    CHECK(!ShouldSwallow(Button(kMouseLeft), CaptureMode::Off, true));
    CHECK(!ShouldSwallow(Button(kMouseLeft), CaptureMode::Armed, true));
    CHECK(!ShouldSwallow(Key(kSpace), CaptureMode::Off, false));
}

PSKB_TEST(GatePassesEverythingWhenApprovedWindowIsNotFocused) {
    CHECK(!ShouldSwallow(Key(kSpace), CaptureMode::Active, false));
    CHECK(!ShouldSwallow(Button(kMouseLeft), CaptureMode::Active, false));
    CHECK(!ShouldSwallow(Button(kMouseLeft), CaptureMode::Armed, false));
}

PSKB_TEST(GatePassesUnmappedKeys) {
    CHECK(!ShouldSwallow(Key(kTab, false), CaptureMode::Active, true));
    CHECK(!ShouldSwallow(Key(kDelete, false), CaptureMode::Active, true));
}

PSKB_TEST(GateSwallowsMouseButtonsAndWheel) {
    CHECK(ShouldSwallow(Button(kMouseLeft), CaptureMode::Active, true));
    CHECK(ShouldSwallow(Button(kMouseRight), CaptureMode::Active, true));
    CHECK(ShouldSwallow(Button(kMouseX1, false), CaptureMode::Active, true));
    GateEvent wheel = Button(kMouseWheel);
    CHECK(ShouldSwallow(wheel, CaptureMode::Active, true));
}

PSKB_TEST(GateSwallowsEscapeWhileActive) {
    CHECK(ShouldSwallow(Key(CaptureSession::kEscape, false), CaptureMode::Active, true));
}

PSKB_TEST(GatePassesEscapeWithCtrl) {
    GateEvent e = Key(CaptureSession::kEscape, false);
    e.ctrlHeld = true;
    CHECK(!ShouldSwallow(e, CaptureMode::Active, true));
}

PSKB_TEST(GatePassesWinKeysAndWinCombinations) {
    GateEvent win = Key(kLeftWinKey, true);
    CHECK(!ShouldSwallow(win, CaptureMode::Active, true));

    GateEvent winSpace = Key(kSpace, true);
    winSpace.winHeld = true;
    CHECK(!ShouldSwallow(winSpace, CaptureMode::Active, true));
}

PSKB_TEST(GatePassesAltTabAndAltCombinations) {
    GateEvent altTab = Key(kTab, true);
    altTab.altHeld = true;
    CHECK(!ShouldSwallow(altTab, CaptureMode::Active, true));

    GateEvent altSpace = Key(kSpace, true);
    altSpace.altHeld = true;
    CHECK(!ShouldSwallow(altSpace, CaptureMode::Active, true));
}

PSKB_TEST(GatePassesInjectedEvents) {
    GateEvent e = Key(kSpace, true);
    e.injected = true;
    CHECK(!ShouldSwallow(e, CaptureMode::Active, true));
    GateEvent m = Button(kMouseLeft);
    m.injected = true;
    CHECK(!ShouldSwallow(m, CaptureMode::Active, true));
}

PSKB_TEST(GatePassesTheHotkey) {
    GateEvent e = Key(kF8, false);
    e.reserved = true;
    CHECK(!ShouldSwallow(e, CaptureMode::Active, true));
}

PSKB_TEST(GatePassesCtrlAltDel) {
    GateEvent ctrlAltDel = Key(kDelete, false);
    ctrlAltDel.ctrlHeld = true;
    ctrlAltDel.altHeld = true;
    CHECK(!ShouldSwallow(ctrlAltDel, CaptureMode::Active, true));
}

PSKB_TEST(GateSwallowsCtrlWhenMappedAndPressedAlone) {
    GateEvent ctrl = Key(kLeftCtrl, true);
    ctrl.ctrlHeld = true;
    CHECK(ShouldSwallow(ctrl, CaptureMode::Active, true));
}

PSKB_TEST(SwallowedKeysReleaseOnlyWhatWasSwallowed) {
    SwallowedKeys keys;
    keys.Remember(kSpace);
    CHECK(keys.Consume(kSpace));
    CHECK(!keys.Consume(kSpace));
    CHECK(!keys.Consume(kW));
    CHECK(keys.Empty());
}

PSKB_TEST(SwallowedKeysClearOnLeavingActive) {
    SwallowedKeys keys;
    keys.Remember(kMouseLeft);
    keys.Remember(kSpace);
    keys.Clear();
    CHECK(keys.Empty());
    CHECK(!keys.Consume(kMouseLeft));
}
