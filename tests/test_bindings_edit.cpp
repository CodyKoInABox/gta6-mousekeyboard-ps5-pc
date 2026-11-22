#include <string>

#include "core/bindings.h"
#include "test_framework.h"

using namespace pskb;

namespace {

constexpr unsigned int kHotkey = 0x77;  // F8

bool HasBinding(const Profile& p, InputCode input, Action action) {
    for (const auto& b : p.bindings) {
        if (b.input == input && b.action == action) return true;
    }
    return false;
}

}  // namespace

PSKB_TEST(BindableInputRejectsEscWinKeysHotkeyAndSideButtons) {
    CHECK(!BindableInputError(0x1B, kHotkey).empty());
    CHECK(!BindableInputError(0x5B, kHotkey).empty());
    CHECK(!BindableInputError(0x5C, kHotkey).empty());
    CHECK(!BindableInputError(kHotkey, kHotkey).empty());
    CHECK(!BindableInputError(kMouseX1, kHotkey).empty());
    CHECK(!BindableInputError(kMouseX2, kHotkey).empty());
}

PSKB_TEST(BindableInputAcceptsLettersMouseButtonsAndNamedKeys) {
    CHECK(BindableInputError('W', kHotkey).empty());
    CHECK(BindableInputError(0x20, kHotkey).empty());
    CHECK(BindableInputError(0xA0, kHotkey).empty());
    CHECK(BindableInputError(kMouseLeft, kHotkey).empty());
    CHECK(BindableInputError(kMouseRight, kHotkey).empty());
    CHECK(BindableInputError(kMouseMiddle, kHotkey).empty());
    CHECK(BindableInputError(0x62, kHotkey).empty());  // Numpad2
}

PSKB_TEST(BindableInputRejectsKeysWithoutStableName) {
    CHECK(!BindableInputError(0xBA, kHotkey).empty());  // OEM ';' has no name in the table
}

PSKB_TEST(InlineBindingsRoundTripForBothProfiles) {
    for (const Profile& original : {GtaProfile(), DrivingProfile()}) {
        const std::string text = SerializeBindingsInline(original);
        CHECK(text.find(';') != std::string::npos);
        Profile parsed = ProfileById(original.id);
        parsed.bindings.clear();
        CHECK(ParseBindingsInline(text, parsed));
        CHECK_EQ(SerializeBindingsInline(parsed), text);
    }
}

PSKB_TEST(InlineBindingsRejectConflictsAndLeaveProfileUnchanged) {
    Profile p = GtaProfile();
    const std::string before = SerializeBindingsInline(p);
    std::string error;
    CHECK(!ParseBindingsInline("W=Cross;W=Circle", p, &error));
    CHECK(!error.empty());
    CHECK_EQ(SerializeBindingsInline(p), before);
}

PSKB_TEST(InlineBindingsRejectUnknownNamesWithoutChangingProfile) {
    Profile p = GtaProfile();
    const std::string before = SerializeBindingsInline(p);
    CHECK(!ParseBindingsInline("NotAKey=cross", p));
    CHECK(!ParseBindingsInline("W=not_an_action", p));
    CHECK(!ParseBindingsInline("", p));
    CHECK_EQ(SerializeBindingsInline(p), before);
}

PSKB_TEST(ProfileWithBindingsAppliesSavedTextAndFallsBackToDefaults) {
    const Profile defaults = GtaProfile();
    const Profile none = ProfileWithBindings("gta6", "");
    CHECK_EQ(SerializeBindingsInline(none), SerializeBindingsInline(defaults));

    const Profile broken = ProfileWithBindings("gta6", "W=nonsense");
    CHECK_EQ(SerializeBindingsInline(broken), SerializeBindingsInline(defaults));

    const Profile custom = ProfileWithBindings("gta6", "F=cross;W=left_stick_up");
    CHECK(HasBinding(custom, 'F', Action::Cross));
    CHECK(!HasBinding(custom, 0x20, Action::Cross));
    CHECK(HasBinding(custom, 'W', Action::LeftStickUp));
}

PSKB_TEST(ProfileByIdPicksDrivingOrGta) {
    CHECK_EQ(ProfileById("driving").id, std::string("driving"));
    CHECK_EQ(ProfileById("gta6").id, std::string("gta6"));
    CHECK_EQ(ProfileById("anything").id, std::string("gta6"));
}

PSKB_TEST(SetBindingInputMovesInputAndReportsDisplacedAction) {
    Profile p = GtaProfile();
    std::size_t rowOfW = p.bindings.size();
    for (std::size_t i = 0; i < p.bindings.size(); ++i) {
        if (p.bindings[i].input == 'W') rowOfW = i;
    }
    CHECK(rowOfW < p.bindings.size());

    const auto displaced = SetBindingInput(p, rowOfW, 'Q');
    CHECK(displaced.has_value());
    CHECK(*displaced == Action::L1);
    CHECK(HasBinding(p, 'Q', Action::LeftStickUp));
    CHECK(!HasBinding(p, 'W', Action::LeftStickUp));
    CHECK(ValidateBindings(p).empty());
}

PSKB_TEST(SetBindingInputToSameInputIsNoop) {
    Profile p = GtaProfile();
    const std::string before = SerializeBindingsInline(p);
    CHECK(!SetBindingInput(p, 0, p.bindings[0].input).has_value());
    CHECK_EQ(SerializeBindingsInline(p), before);
}

PSKB_TEST(AddBindingCreatesRowAndDisplacesOtherAction) {
    Profile p = GtaProfile();
    const std::size_t before = p.bindings.size();
    CHECK(!AddBinding(p, Action::Ps, 0x2D).has_value());  // Insert is free
    CHECK_EQ(p.bindings.size(), before + 1);

    const auto displaced = AddBinding(p, Action::Ps, 'E');  // E is Triangle
    CHECK(displaced.has_value());
    CHECK(*displaced == Action::Triangle);
    CHECK(HasBinding(p, 'E', Action::Ps));
    CHECK(ValidateBindings(p).empty());
}

PSKB_TEST(AddBindingSameActionSameInputDoesNotDuplicate) {
    Profile p = GtaProfile();
    const std::size_t before = p.bindings.size();
    CHECK(!AddBinding(p, Action::Cross, 0x20).has_value());
    CHECK_EQ(p.bindings.size(), before);
}

PSKB_TEST(RemoveBindingRefusesToEmptyProfile) {
    Profile p = GtaProfile();
    while (p.bindings.size() > 1) CHECK(RemoveBinding(p, 0));
    CHECK(!RemoveBinding(p, 0));
    CHECK_EQ(p.bindings.size(), std::size_t{1});
    CHECK(!RemoveBinding(p, 99));
}

PSKB_TEST(ActionAndInputLabelsAreReadable) {
    for (std::size_t i = 0; i < kActionCount; ++i) {
        CHECK(!ActionLabel(static_cast<Action>(i)).empty());
    }
    CHECK_EQ(InputLabel(kMouseLeft), std::string("Mouse: botão esquerdo"));
    CHECK_EQ(InputLabel('W'), std::string("W"));
}
