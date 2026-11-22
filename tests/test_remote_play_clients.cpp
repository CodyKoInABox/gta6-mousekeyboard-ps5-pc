#include <string>

#include "core/remote_play_clients.h"
#include "test_framework.h"

using namespace pskb;

PSKB_TEST(OfficialRemotePlayAppIsAClient) {
    CHECK(IsRemotePlayClientExe(L"C:\\Program Files\\Sony\\PS Remote Play\\PS Remote Play.exe"));
}

PSKB_TEST(ChiakiBuildsAreClients) {
    CHECK(IsRemotePlayClientExe(L"C:\\Tools\\chiaki.exe"));
    CHECK(IsRemotePlayClientExe(L"C:\\Tools\\Chiaki-ng.exe"));
}

PSKB_TEST(VeyraAndOurOwnToolsAreNotClients) {
    CHECK(!IsRemotePlayClientExe(L"C:\\Program Files\\Veyra\\Veyra.exe"));
    CHECK(!IsRemotePlayClientExe(L"C:\\Users\\me\\PS5KeyboardBridge.exe"));
    CHECK(!IsRemotePlayClientExe(L"C:\\Users\\me\\ps5kb-cli.exe"));
}

PSKB_TEST(NamesAreMatchedWholeNotAsSubstrings) {
    CHECK(!IsRemotePlayClientExe(L"C:\\Tools\\chiaki-tools.exe"));
    CHECK(!IsRemotePlayClientExe(L"C:\\Games\\remoteplayer.exe"));
}

PSKB_TEST(EmptyPathIsNotAClient) {
    CHECK(!IsRemotePlayClientExe(L""));
}
