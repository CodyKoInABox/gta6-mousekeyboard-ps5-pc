#include "core/pad_engine.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace pskb {
namespace {

constexpr double kMaxStepMs = 200.0;

std::uint32_t ButtonFor(Action action) noexcept {
    switch (action) {
        case Action::Cross: return PadCross;
        case Action::Circle: return PadCircle;
        case Action::Square: return PadSquare;
        case Action::Triangle: return PadTriangle;
        case Action::DpadUp: return PadDpadUp;
        case Action::DpadDown: return PadDpadDown;
        case Action::DpadLeft: return PadDpadLeft;
        case Action::DpadRight: return PadDpadRight;
        case Action::L1: return PadL1;
        case Action::R1: return PadR1;
        case Action::L3: return PadL3;
        case Action::R3: return PadR3;
        case Action::Options: return PadOptions;
        case Action::Share: return PadShare;
        case Action::Touchpad: return PadTouchpad;
        case Action::Ps: return PadPs;
        default: return 0;
    }
}

}  // namespace

PadEngine::PadEngine(Profile profile) : profile_(std::move(profile)) {
    mouse_.Configure(profile_.aim);
}

void PadEngine::SetProfile(Profile profile) {
    profile_ = std::move(profile);
    mouse_.Configure(profile_.aim);
    held_.clear();
    mouse_.Reset();
    leftX_ = leftY_ = l2_ = r2_ = 0.0f;
    lastUpdateMs_ = -1.0;
    last_ = PadState{};
}

void PadEngine::KeyDown(InputCode code) { held_.insert(code); }

void PadEngine::KeyUp(InputCode code) { held_.erase(code); }

void PadEngine::MouseMove(int dx, int dy, double nowMs) { mouse_.AddMotion(nowMs, dx, dy); }

void PadEngine::ReleaseAll() {
    held_.clear();
    mouse_.Reset();
    leftX_ = leftY_ = l2_ = r2_ = 0.0f;
    last_ = PadState{};
}

bool PadEngine::IsHeld(InputCode code) const { return held_.count(code) != 0; }

float PadEngine::Approach(float current, float target, float maxStep) noexcept {
    if (target > current) return std::min(target, current + maxStep);
    return std::max(target, current - maxStep);
}

void PadEngine::RebuildActiveActions() {
    active_.reset();
    for (const auto& binding : profile_.bindings) {
        if (held_.count(binding.input)) active_.set(static_cast<std::size_t>(binding.action));
    }
}

PadState PadEngine::Update(double nowMs) {
    const double dtMs = lastUpdateMs_ < 0.0 ? 0.0 : std::clamp(nowMs - lastUpdateMs_, 0.0, kMaxStepMs);
    lastUpdateMs_ = nowMs;

    RebuildActiveActions();
    auto on = [&](Action a) { return active_.test(static_cast<std::size_t>(a)); };

    std::uint32_t buttons = 0;
    for (std::size_t i = 0; i < kActionCount; ++i) {
        if (active_.test(i)) buttons |= ButtonFor(static_cast<Action>(i));
    }

    const float dx = (on(Action::LeftStickRight) ? 1.0f : 0.0f) - (on(Action::LeftStickLeft) ? 1.0f : 0.0f);
    const float dy = (on(Action::LeftStickDown) ? 1.0f : 0.0f) - (on(Action::LeftStickUp) ? 1.0f : 0.0f);
    const float length = std::sqrt(dx * dx + dy * dy);
    const float targetX = length > 0.0f ? dx / length : 0.0f;
    const float targetY = length > 0.0f ? dy / length : 0.0f;
    const float stickStep = profile_.stickRampMs > 0.0f
                                ? static_cast<float>(dtMs / profile_.stickRampMs)
                                : std::numeric_limits<float>::infinity();
    leftX_ = Approach(leftX_, targetX, stickStep);
    leftY_ = Approach(leftY_, targetY, stickStep);

    const float l2Goal = on(Action::L2) ? 1.0f : 0.0f;
    const float r2Goal = on(Action::R2) ? 1.0f : 0.0f;
    const float triggerStep = profile_.triggerRampMs > 0.0f
                                  ? static_cast<float>(dtMs / profile_.triggerRampMs)
                                  : std::numeric_limits<float>::infinity();
    l2_ = Approach(l2_, l2Goal, triggerStep);
    r2_ = Approach(r2_, r2Goal, triggerStep);

    const StickVector aim = mouse_.Evaluate(nowMs);

    PadState out;
    out.buttons = buttons;
    out.l2 = TriggerFromUnit(l2_);
    out.r2 = TriggerFromUnit(r2_);
    out.lx = StickFromUnit(leftX_);
    out.ly = StickFromUnit(leftY_);
    out.rx = StickFromUnit(aim.x);
    out.ry = StickFromUnit(aim.y);
    last_ = out;
    return out;
}

}  // namespace pskb
