#include "backend/chiaki_backend.h"
#include "chiaki/controller.h"
#include "chiaki/session.h"
#include "test_framework.h"

using namespace pskb;

namespace {

template <typename A, typename B>
constexpr bool SameBit(A a, B b) {
    return static_cast<std::uint32_t>(a) == static_cast<std::uint32_t>(b);
}

}  // namespace

// Protocol bit positions must match ours exactly, or buttons would land on the wrong keys.
static_assert(SameBit(PadCross, CHIAKI_CONTROLLER_BUTTON_CROSS));
static_assert(SameBit(PadCircle, CHIAKI_CONTROLLER_BUTTON_MOON));
static_assert(SameBit(PadSquare, CHIAKI_CONTROLLER_BUTTON_BOX));
static_assert(SameBit(PadTriangle, CHIAKI_CONTROLLER_BUTTON_PYRAMID));
static_assert(SameBit(PadDpadLeft, CHIAKI_CONTROLLER_BUTTON_DPAD_LEFT));
static_assert(SameBit(PadDpadRight, CHIAKI_CONTROLLER_BUTTON_DPAD_RIGHT));
static_assert(SameBit(PadDpadUp, CHIAKI_CONTROLLER_BUTTON_DPAD_UP));
static_assert(SameBit(PadDpadDown, CHIAKI_CONTROLLER_BUTTON_DPAD_DOWN));
static_assert(SameBit(PadL1, CHIAKI_CONTROLLER_BUTTON_L1));
static_assert(SameBit(PadR1, CHIAKI_CONTROLLER_BUTTON_R1));
static_assert(SameBit(PadL3, CHIAKI_CONTROLLER_BUTTON_L3));
static_assert(SameBit(PadR3, CHIAKI_CONTROLLER_BUTTON_R3));
static_assert(SameBit(PadOptions, CHIAKI_CONTROLLER_BUTTON_OPTIONS));
static_assert(SameBit(PadShare, CHIAKI_CONTROLLER_BUTTON_SHARE));
static_assert(SameBit(PadTouchpad, CHIAKI_CONTROLLER_BUTTON_TOUCHPAD));
static_assert(SameBit(PadPs, CHIAKI_CONTROLLER_BUTTON_PS));

PSKB_TEST(ControllerStateCarriesEveryField) {
    PadState pad;
    pad.buttons = PadCross | PadL1 | PadDpadUp;
    pad.l2 = 10;
    pad.r2 = 255;
    pad.lx = -1000;
    pad.ly = 2000;
    pad.rx = 32767;
    pad.ry = -32767;

    chiaki_controller_state_t out{};
    chiaki_backend::ToControllerState(pad, out);

    CHECK_EQ(out.buttons, static_cast<std::uint32_t>(pad.buttons));
    CHECK_EQ(static_cast<int>(out.l2_state), 10);
    CHECK_EQ(static_cast<int>(out.r2_state), 255);
    CHECK_EQ(static_cast<int>(out.left_x), -1000);
    CHECK_EQ(static_cast<int>(out.left_y), 2000);
    CHECK_EQ(static_cast<int>(out.right_x), 32767);
    CHECK_EQ(static_cast<int>(out.right_y), -32767);
}

PSKB_TEST(NeutralPadMapsToIdleControllerState) {
    chiaki_controller_state_t out{};
    out.buttons = 0xFFFF;
    out.left_x = 5;
    chiaki_backend::ToControllerState(PadState{}, out);
    CHECK_EQ(out.buttons, static_cast<std::uint32_t>(0));
    CHECK_EQ(static_cast<int>(out.left_x), 0);
    CHECK_EQ(static_cast<int>(out.right_y), 0);
}

PSKB_TEST(QuitReasonsClassifyForRetryPolicy) {
    CHECK(chiaki_backend::ClassifyQuit(CHIAKI_QUIT_REASON_STOPPED) == LinkError::Cancelled);
    CHECK(chiaki_backend::ClassifyQuit(CHIAKI_QUIT_REASON_SESSION_REQUEST_CONNECTION_REFUSED) == LinkError::Refused);
    CHECK(chiaki_backend::ClassifyQuit(CHIAKI_QUIT_REASON_SESSION_REQUEST_RP_IN_USE) == LinkError::Refused);
    CHECK(chiaki_backend::ClassifyQuit(CHIAKI_QUIT_REASON_PSN_REGIST_FAILED) == LinkError::NotPaired);
    CHECK(chiaki_backend::ClassifyQuit(CHIAKI_QUIT_REASON_STREAM_CONNECTION_REMOTE_DISCONNECTED) == LinkError::Protocol);
}

PSKB_TEST(QuitMessagesAreUserFacingAndNeverEmpty) {
    for (int reason = CHIAKI_QUIT_REASON_NONE; reason <= CHIAKI_QUIT_REASON_PSN_REGIST_FAILED; ++reason) {
        CHECK(!chiaki_backend::QuitMessage(reason).empty());
    }
    CHECK(chiaki_backend::QuitMessage(CHIAKI_QUIT_REASON_SESSION_REQUEST_RP_IN_USE).find("PS Remote Play") !=
          std::string::npos);
}
