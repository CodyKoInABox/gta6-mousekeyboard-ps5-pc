#include "core/process_match.h"
#include "test_framework.h"

using namespace pskb;

PSKB_TEST(MatchesFullPathAndPlainPattern) {
    CHECK(ProcessNameMatches(L"C:\\Program Files\\Veyra\\Veyra.exe", L"Veyra"));
    CHECK(ProcessNameMatches(L"D:/apps/veyra.exe", L"Veyra"));
}

PSKB_TEST(MatchingIgnoresCaseAndExtension) {
    CHECK(ProcessNameMatches(L"veyra.exe", L"VEYRA.EXE"));
    CHECK(ProcessNameMatches(L"Veyra", L"veyra"));
}

PSKB_TEST(PatternMayBeAPath) {
    CHECK(ProcessNameMatches(L"C:\\Veyra\\Veyra.exe", L"C:\\Other\\Veyra.exe"));
}

PSKB_TEST(RejectsOtherProcesses) {
    CHECK(!ProcessNameMatches(L"C:\\x\\NotVeyra.exe", L"Veyra"));
    CHECK(!ProcessNameMatches(L"C:\\x\\Veyra2.exe", L"Veyra"));
    CHECK(!ProcessNameMatches(L"C:\\x\\Veyra.exe.bak", L"Veyra"));
    CHECK(!ProcessNameMatches(L"explorer.exe", L"Veyra"));
}

PSKB_TEST(EmptyPatternNeverMatches) {
    CHECK(!ProcessNameMatches(L"Veyra.exe", L""));
    CHECK(!ProcessNameMatches(L"Veyra.exe", L".exe"));
    CHECK(!ProcessNameMatches(L"", L""));
}
