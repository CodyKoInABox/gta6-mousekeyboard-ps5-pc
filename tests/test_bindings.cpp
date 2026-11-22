#include "core/bindings.h"
#include "core/input_codes.h"
#include "test_framework.h"

using namespace pskb;

namespace {

const Binding* Find(const Profile& p, InputCode code) {
    for (const auto& b : p.bindings) {
        if (b.input == code) return &b;
    }
    return nullptr;
}

}  // namespace

PSKB_TEST(GtaDefaultsMatchBrief) {
    const Profile p = GtaProfile();
    CHECK(Find(p, 'W') && Find(p, 'W')->action == Action::LeftStickUp);
    CHECK(Find(p, 'A') && Find(p, 'A')->action == Action::LeftStickLeft);
    CHECK(Find(p, 'S') && Find(p, 'S')->action == Action::LeftStickDown);
    CHECK(Find(p, 'D') && Find(p, 'D')->action == Action::LeftStickRight);
    CHECK(Find(p, kMouseLeft) && Find(p, kMouseLeft)->action == Action::R2);
    CHECK(Find(p, kMouseRight) && Find(p, kMouseRight)->action == Action::L2);
    CHECK(Find(p, 0x20) && Find(p, 0x20)->action == Action::Cross);
    CHECK(Find(p, 0xA2) && Find(p, 0xA2)->action == Action::Circle);
    CHECK(Find(p, 'E') && Find(p, 'E')->action == Action::Triangle);
    CHECK(Find(p, 'R') && Find(p, 'R')->action == Action::Square);
    CHECK(Find(p, 0xA0) && Find(p, 0xA0)->action == Action::L3);
    CHECK(Find(p, kMouseMiddle) && Find(p, kMouseMiddle)->action == Action::R3);
    CHECK(Find(p, 'Q') && Find(p, 'Q')->action == Action::L1);
    CHECK(Find(p, 'F') && Find(p, 'F')->action == Action::R1);
    CHECK(Find(p, 0x09) && Find(p, 0x09)->action == Action::Touchpad);
    CHECK(Find(p, 0x0D) && Find(p, 0x0D)->action == Action::Options);
    CHECK(Find(p, 0x26) && Find(p, 0x26)->action == Action::DpadUp);
    CHECK(Find(p, 0x28) && Find(p, 0x28)->action == Action::DpadDown);
    CHECK(Find(p, 0x25) && Find(p, 0x25)->action == Action::DpadLeft);
    CHECK(Find(p, 0x27) && Find(p, 0x27)->action == Action::DpadRight);
}

PSKB_TEST(GtaDefaultsAreValid) {
    CHECK(ValidateBindings(GtaProfile()).empty());
    CHECK(ValidateBindings(DrivingProfile()).empty());
}

PSKB_TEST(DrivingProfileRampsSteering) {
    const Profile p = DrivingProfile();
    CHECK(p.stickRampMs > 0.0f);
    CHECK(p.triggerRampMs > 0.0f);
    CHECK(Find(p, 'W') && Find(p, 'W')->action == Action::R2);
}

PSKB_TEST(ActionNamesRoundTrip) {
    for (std::size_t i = 0; i < kActionCount; ++i) {
        const auto action = static_cast<Action>(i);
        const auto name = ActionToName(action);
        const auto parsed = ActionFromName(name);
        CHECK(parsed.has_value());
        CHECK(parsed.value() == action);
    }
}

PSKB_TEST(ActionNameLookupIsCaseAndSpaceTolerant) {
    CHECK(ActionFromName("  CROSS ").has_value());
    CHECK(ActionFromName("left_stick_up").value() == Action::LeftStickUp);
    CHECK(!ActionFromName("launch_missiles").has_value());
}

PSKB_TEST(InputNamesResolve) {
    CHECK(InputCodeFromName("w").value() == 'W');
    CHECK(InputCodeFromName("W").value() == 'W');
    CHECK(InputCodeFromName("9").value() == '9');
    CHECK(InputCodeFromName("space").value() == 0x20);
    CHECK(InputCodeFromName("LCtrl").value() == 0xA2);
    CHECK(InputCodeFromName("F8").value() == 0x77);
    CHECK(InputCodeFromName("MouseLeft").value() == kMouseLeft);
    CHECK(!InputCodeFromName("NotAKey").has_value());
    CHECK(!InputCodeFromName("").has_value());
}

PSKB_TEST(InputNamesRoundTrip) {
    for (const InputCode code : {static_cast<InputCode>('Q'), static_cast<InputCode>(0x20),
                                 static_cast<InputCode>(0x77), kMouseRight}) {
        const auto name = InputCodeToName(code);
        CHECK(InputCodeFromName(name).value() == code);
    }
    CHECK(IsMouseButtonCode(kMouseX2));
    CHECK(!IsMouseButtonCode('A'));
}

PSKB_TEST(BindingsSerializeAndParseRoundTrip) {
    const Profile original = DrivingProfile();
    Profile loaded = GtaProfile();
    std::string error;
    CHECK(ParseBindings(SerializeBindings(original), loaded, &error));
    CHECK(error.empty());
    CHECK_EQ(loaded.bindings.size(), original.bindings.size());
    for (std::size_t i = 0; i < loaded.bindings.size(); ++i) {
        CHECK(loaded.bindings[i].input == original.bindings[i].input);
        CHECK(loaded.bindings[i].action == original.bindings[i].action);
    }
}

PSKB_TEST(ParseBindingsReportsBadLinesAndKeepsProfile) {
    Profile p = GtaProfile();
    const auto before = p.bindings.size();
    std::string error;
    const bool ok = ParseBindings("W=left_stick_up\nBogusKey=cross\nSpace=launch\nnonsense\n", p, &error);
    CHECK(!ok);
    CHECK(error.find("line 2") != std::string::npos);
    CHECK(error.find("line 3") != std::string::npos);
    CHECK(error.find("line 4") != std::string::npos);
    CHECK_EQ(p.bindings.size(), before);
}

PSKB_TEST(ParseBindingsIgnoresCommentsAndBlankLines) {
    Profile p = GtaProfile();
    CHECK(ParseBindings("# comment\n\n  W = cross \n", p));
    CHECK_EQ(p.bindings.size(), static_cast<std::size_t>(1));
    CHECK(p.bindings[0].action == Action::Cross);
}

PSKB_TEST(ParseBindingsRejectsEmptyProfile) {
    Profile p = GtaProfile();
    CHECK(!ParseBindings("# nothing here\n", p));
}

PSKB_TEST(ValidateBindingsFlagsInputBoundTwice) {
    Profile p;
    p.bindings = {{'W', Action::LeftStickUp}, {'W', Action::Cross}};
    CHECK(!ValidateBindings(p).empty());
}

PSKB_TEST(ValidateBindingsAllowsSeveralInputsForOneAction) {
    Profile p;
    p.bindings = {{'W', Action::LeftStickUp}, {0x26, Action::LeftStickUp}};
    CHECK(ValidateBindings(p).empty());
}
