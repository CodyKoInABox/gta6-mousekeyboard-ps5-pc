#include "core/mouse_to_stick.h"

#include <algorithm>
#include <cmath>

namespace pskb {
namespace {

float Clamp01(float v) { return std::clamp(v, 0.0f, 1.0f); }

}  // namespace

MouseToStick::MouseToStick(AimSettings settings) { Configure(settings); }

void MouseToStick::Configure(const AimSettings& settings) {
    settings_ = settings;
    settings_.sensitivityX = std::clamp(settings_.sensitivityX, 0.05f, 10.0f);
    settings_.sensitivityY = std::clamp(settings_.sensitivityY, 0.05f, 10.0f);
    settings_.aimSensitivityX = std::clamp(settings_.aimSensitivityX, 0.05f, 10.0f);
    settings_.aimSensitivityY = std::clamp(settings_.aimSensitivityY, 0.05f, 10.0f);
    settings_.deadzone = std::clamp(settings_.deadzone, 0.0f, 0.5f);
    settings_.antiDeadzone = std::clamp(settings_.antiDeadzone, 0.0f, 0.5f);
    settings_.curveExponent = std::clamp(settings_.curveExponent, 1.0f, 3.0f);
    settings_.acceleration = std::clamp(settings_.acceleration, 0.0f, 1.0f);
    settings_.smoothingMs = std::clamp(settings_.smoothingMs, 0.0f, 200.0f);
    settings_.recenterMs = std::clamp(settings_.recenterMs, 10.0f, 1000.0f);
    settings_.windowMs = std::clamp(settings_.windowMs, 4.0f, 40.0f);
    settings_.fullDeflectionCountsPerSec =
        std::clamp(settings_.fullDeflectionCountsPerSec, 100.0f, 50000.0f);
}

void MouseToStick::Reset() {
    head_ = count_ = 0;
    sumX_ = sumY_ = 0;
    lastMotionMs_ = -1e18;
    lastEvalMs_ = -1.0;
    smoothed_ = {};
}

void MouseToStick::AddMotion(double timeMs, int dx, int dy) {
    if (dx == 0 && dy == 0) return;
    if (count_ == kCapacity) {
        const Sample& oldest = ring_[head_];
        sumX_ -= oldest.dx;
        sumY_ -= oldest.dy;
        head_ = (head_ + 1) % kCapacity;
        --count_;
    }
    ring_[(head_ + count_) % kCapacity] = {timeMs, dx, dy};
    ++count_;
    sumX_ += dx;
    sumY_ += dy;
    lastMotionMs_ = timeMs;
}

void MouseToStick::Expire(double cutoffMs) {
    while (count_ > 0 && ring_[head_].timeMs <= cutoffMs) {
        sumX_ -= ring_[head_].dx;
        sumY_ -= ring_[head_].dy;
        head_ = (head_ + 1) % kCapacity;
        --count_;
    }
}

float MouseToStick::ShapeMagnitude(float magnitude, const AimSettings& s) {
    float m = Clamp01(magnitude);
    if (m <= 0.0f) return 0.0f;
    m = std::min(1.0f, m * (1.0f + s.acceleration * m));
    if (m <= s.deadzone) return 0.0f;
    m = (m - s.deadzone) / (1.0f - s.deadzone);
    if (s.exponentialCurve) m = std::pow(m, s.curveExponent);
    return s.antiDeadzone + (1.0f - s.antiDeadzone) * m;
}

StickVector MouseToStick::Evaluate(double timeMs, bool aiming) {
    const double dtMs = lastEvalMs_ < 0.0 ? 0.0 : std::max(0.0, timeMs - lastEvalMs_);
    lastEvalMs_ = timeMs;

    Expire(timeMs - settings_.windowMs);

    const float sensX = aiming ? settings_.aimSensitivityX : settings_.sensitivityX;
    const float sensY = aiming ? settings_.aimSensitivityY : settings_.sensitivityY;

    StickVector target{};
    const bool recentlyMoved = timeMs - lastMotionMs_ <= settings_.recenterMs;
    if (count_ > 0 && recentlyMoved) {
        const double window = settings_.windowMs;
        const float vx = static_cast<float>(sumX_ / window) * sensX;
        float vy = static_cast<float>(sumY_ / window) * sensY;
        if (settings_.invertY) vy = -vy;
        const float countsPerSec = std::sqrt(vx * vx + vy * vy) * 1000.0f;
        if (countsPerSec > 0.0f) {
            const float magnitude = countsPerSec / settings_.fullDeflectionCountsPerSec;
            const float shaped = ShapeMagnitude(magnitude, settings_);
            const float length = countsPerSec / 1000.0f;
            target = {vx / length * shaped, vy / length * shaped};
        }
    }

    if (settings_.smoothingMs > 0.0f && dtMs > 0.0) {
        const float alpha = static_cast<float>(1.0 - std::exp(-dtMs / settings_.smoothingMs));
        smoothed_.x += alpha * (target.x - smoothed_.x);
        smoothed_.y += alpha * (target.y - smoothed_.y);
    } else {
        smoothed_ = target;
    }

    StickVector out = smoothed_;
    const float length = std::sqrt(out.x * out.x + out.y * out.y);
    if (length > 1.0f) {
        out.x /= length;
        out.y /= length;
    }
    return out;
}

}  // namespace pskb
