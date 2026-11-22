#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "core/bindings.h"
#include "platform/input_bridge.h"
#include "test_framework.h"

using namespace pskb;

namespace {

template <typename Predicate>
bool WaitFor(Predicate predicate, int timeoutMs = 3000) {
    const auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (std::chrono::steady_clock::now() < end) {
        if (predicate()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return predicate();
}

}  // namespace

// Starts the real window and Raw Input registration on this desktop. No input is injected: the
// test only checks that the thread runs, mode changes propagate, and shutdown is clean.
PSKB_TEST(BridgeStartsTogglesDisarmsAndStops) {
    std::atomic<int> frames{0};
    std::mutex modesMutex;
    std::vector<CaptureMode> modes;

    InputBridge bridge(
        GtaProfile(), CaptureOptions{}, [&](const PadState&) { ++frames; },
        [&](CaptureMode mode) {
            std::lock_guard lock(modesMutex);
            modes.push_back(mode);
        });

    std::string error;
    if (!bridge.Start(&error)) pskbtest::Fail(__FILE__, __LINE__, "Start failed: " + error);
    CHECK(WaitFor([&] { return frames.load() > 5; }));
    CHECK(bridge.Mode() == CaptureMode::Off);

    bridge.Toggle();
    CHECK(WaitFor([&] { return bridge.Mode() == CaptureMode::Armed; }));

    bridge.Disarm();
    CHECK(WaitFor([&] { return bridge.Mode() == CaptureMode::Off; }));

    bridge.Stop();
    CHECK(bridge.Mode() == CaptureMode::Off);

    std::lock_guard lock(modesMutex);
    CHECK(modes.size() >= 2);
    CHECK(modes.front() == CaptureMode::Armed);
    CHECK(modes.back() == CaptureMode::Off);
}
