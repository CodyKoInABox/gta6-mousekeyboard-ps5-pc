#include <string>

#include "core/windows_support.h"
#include "test_framework.h"

using namespace pskb;

namespace {

WindowsVersion Win(unsigned int major, unsigned int minor, unsigned int build, bool is64 = true) {
    WindowsVersion v;
    v.major = major;
    v.minor = minor;
    v.build = build;
    v.is64Bit = is64;
    return v;
}

}  // namespace

PSKB_TEST(WindowsSupportTreatsWindows10AtOrAfter1809AsSupported) {
    CHECK(CheckWindowsSupport(Win(10, 0, 19045)) == WindowsSupport::Supported);  // development PC
    CHECK(CheckWindowsSupport(Win(10, 0, 17763)) == WindowsSupport::Supported);
}

PSKB_TEST(WindowsSupportTreatsWindows11AsSupported) {
    CHECK(CheckWindowsSupport(Win(10, 0, 22000)) == WindowsSupport::Supported);
    CHECK(CheckWindowsSupport(Win(10, 0, 26100)) == WindowsSupport::Supported);
}

PSKB_TEST(WindowsSupportTreatsOlderWindows10AsUntested) {
    CHECK(CheckWindowsSupport(Win(10, 0, 17762)) == WindowsSupport::Untested);
    CHECK(CheckWindowsSupport(Win(10, 0, 10240)) == WindowsSupport::Untested);
}

PSKB_TEST(WindowsSupportRejectsWindows8AndOlder) {
    CHECK(CheckWindowsSupport(Win(6, 3, 9600)) == WindowsSupport::Unsupported);
    CHECK(CheckWindowsSupport(Win(6, 1, 7601)) == WindowsSupport::Unsupported);
}

PSKB_TEST(WindowsSupportRejects32BitSystems) {
    CHECK(CheckWindowsSupport(Win(10, 0, 19045, false)) == WindowsSupport::Unsupported);
}

PSKB_TEST(WindowsSupportMessageIsEmptyWhenSupportedAndNamesThresholdWhenUntested) {
    CHECK(WindowsSupportMessage(Win(10, 0, 19045), WindowsSupport::Supported).empty());

    const std::string untested = WindowsSupportMessage(Win(10, 0, 17762), WindowsSupport::Untested);
    CHECK(untested.find("17762") != std::string::npos);
    CHECK(untested.find("1809") != std::string::npos);

    const std::string unsupported = WindowsSupportMessage(Win(6, 3, 9600), WindowsSupport::Unsupported);
    CHECK(unsupported.find("64 bits") != std::string::npos);
}
