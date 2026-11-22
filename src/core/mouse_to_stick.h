#pragma once

#include <array>
#include <cstdint>

namespace pskb {

struct AimSettings {
    float sensitivityX = 1.0f;     // multiplier, 0.1..5
    float sensitivityY = 1.0f;     // multiplier, 0.1..5
    bool invertY = false;
    float deadzone = 0.02f;        // fraction of full deflection ignored near centre, 0..0.5
    float antiDeadzone = 0.10f;    // fraction of full deflection applied as soon as the stick moves, 0..0.5
    bool exponentialCurve = false; // false: linear response, true: power curve
    float curveExponent = 1.6f;    // used by the power curve, 1..3
    float acceleration = 0.0f;     // extra gain for fast movement, 0..1
    float smoothingMs = 0.0f;      // 0 = none; otherwise time constant of the output filter
    float recenterMs = 90.0f;      // no mouse motion for this long returns the stick to centre
    float windowMs = 16.0f;        // velocity averaging window, 4..40
    float fullDeflectionCountsPerSec = 2400.0f; // mouse speed that gives full stick at sensitivity 1
};

struct StickVector {
    float x = 0.0f;
    float y = 0.0f;
};

// Turns relative mouse motion into a right-stick vector.
// Output depends only on timestamps, never on how often Evaluate() is called, so the
// result is the same at 60 or 240 frames per second.
class MouseToStick {
public:
    explicit MouseToStick(AimSettings settings = {});

    void Configure(const AimSettings& settings);
    const AimSettings& Settings() const noexcept { return settings_; }

    void AddMotion(double timeMs, int dx, int dy);
    StickVector Evaluate(double timeMs);
    void Reset();

    // Exposed for tests and the response-curve preview in the settings screen.
    static float ShapeMagnitude(float magnitude, const AimSettings& settings);

private:
    struct Sample {
        double timeMs;
        int dx;
        int dy;
    };

    void Expire(double cutoffMs);

    static constexpr std::size_t kCapacity = 256;

    AimSettings settings_;
    std::array<Sample, kCapacity> ring_{};
    std::size_t head_ = 0;
    std::size_t count_ = 0;
    long long sumX_ = 0;
    long long sumY_ = 0;
    double lastMotionMs_ = -1e18;
    double lastEvalMs_ = -1.0;
    StickVector smoothed_{};
};

}  // namespace pskb
