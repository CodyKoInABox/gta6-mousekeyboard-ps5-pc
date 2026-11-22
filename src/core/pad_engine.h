#pragma once

#include <bitset>
#include <unordered_set>

#include "core/bindings.h"
#include "core/input_codes.h"
#include "core/mouse_to_stick.h"
#include "core/pad_state.h"

namespace pskb {

// Turns the set of physically held inputs plus mouse motion into controller frames.
//
// Not thread-safe: the input thread owns it and publishes the PadState from Update().
//
// Safety: a key that goes up releases its action on the next Update(). ReleaseAll()
// (focus loss, disarm, emergency release) drops every held input and recentres both sticks
// at once, without ramping.
class PadEngine {
public:
    explicit PadEngine(Profile profile = GtaProfile());

    void SetProfile(Profile profile);
    const Profile& CurrentProfile() const noexcept { return profile_; }

    void KeyDown(InputCode code);
    void KeyUp(InputCode code);
    void MouseMove(int dx, int dy, double nowMs);

    void ReleaseAll();

    bool IsHeld(InputCode code) const;
    std::size_t HeldCount() const noexcept { return held_.size(); }

    PadState Update(double nowMs);
    PadState LastState() const noexcept { return last_; }

private:
    void RebuildActiveActions();
    static float Approach(float current, float target, float maxStep) noexcept;

    Profile profile_;
    MouseToStick mouse_;
    std::unordered_set<InputCode> held_;
    std::bitset<kActionCount> active_;
    float leftX_ = 0.0f;
    float leftY_ = 0.0f;
    float l2_ = 0.0f;
    float r2_ = 0.0f;
    double lastUpdateMs_ = -1.0;
    PadState last_;
};

}  // namespace pskb
