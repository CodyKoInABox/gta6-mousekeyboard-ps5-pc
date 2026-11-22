#include <windows.h>

#include <filesystem>
#include <fstream>
#include <iterator>

#include "core/secure_store.h"
#include "test_framework.h"

using namespace pskb;

namespace {

std::vector<std::uint8_t> Bytes(const char* s) {
    return std::vector<std::uint8_t>(s, s + std::char_traits<char>::length(s));
}

std::filesystem::path TempDir(const char* leaf) {
    wchar_t buf[MAX_PATH]{};
    GetTempPathW(MAX_PATH, buf);
    auto dir = std::filesystem::path(buf) / L"pskb-tests" / std::filesystem::path(leaf);
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    return dir;
}

}  // namespace

PSKB_TEST(DpapiRoundTripRestoresPlaintext) {
    const auto plain = Bytes("regist-key-and-morning-secret");
    std::vector<std::uint8_t> cipher;
    CHECK(secure_store::Protect(plain, cipher));
    CHECK(cipher != plain);
    std::vector<std::uint8_t> back;
    CHECK(secure_store::Unprotect(cipher, back));
    CHECK(back == plain);
}

PSKB_TEST(DpapiRejectsEmptyAndTamperedInput) {
    std::vector<std::uint8_t> cipher;
    CHECK(!secure_store::Protect({}, cipher));

    const auto plain = Bytes("secret");
    CHECK(secure_store::Protect(plain, cipher));
    cipher[cipher.size() / 2] ^= 0x5A;
    std::vector<std::uint8_t> back;
    CHECK(!secure_store::Unprotect(cipher, back));
}

PSKB_TEST(SaveFileWritesEncryptedBlobThatLoadsBack) {
    const auto dir = TempDir("save-load");
    const auto path = dir / "pairing.bin";
    const auto plain = Bytes("pairing-blob-v1");
    CHECK(secure_store::SaveFile(path, plain));
    CHECK(std::filesystem::exists(path));

    std::ifstream raw(path, std::ios::binary);
    std::string onDisk((std::istreambuf_iterator<char>(raw)), std::istreambuf_iterator<char>());
    CHECK(onDisk.find("pairing-blob-v1") == std::string::npos);

    const auto loaded = secure_store::LoadFile(path);
    CHECK(loaded.has_value());
    CHECK(loaded.value() == plain);
    CHECK(!std::filesystem::exists(path.string() + ".tmp"));
}

PSKB_TEST(SaveFileReplacesExistingContent) {
    const auto dir = TempDir("replace");
    const auto path = dir / "pairing.bin";
    CHECK(secure_store::SaveFile(path, Bytes("first")));
    CHECK(secure_store::SaveFile(path, Bytes("second")));
    CHECK(secure_store::LoadFile(path).value() == Bytes("second"));
}

PSKB_TEST(LoadFileReturnsNothingForMissingOrCorruptFiles) {
    const auto dir = TempDir("missing");
    CHECK(!secure_store::LoadFile(dir / "absent.bin").has_value());

    const auto corrupt = dir / "corrupt.bin";
    std::ofstream(corrupt, std::ios::binary) << "not dpapi data at all";
    CHECK(!secure_store::LoadFile(corrupt).has_value());
}
