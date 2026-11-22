#include "core/secure_store.h"

#include <windows.h>
#include <wincrypt.h>

#include <cstring>
#include <fstream>
#include <iterator>
#include <system_error>

namespace pskb::secure_store {
namespace {

constexpr char kEntropy[] = "PS5KeyboardBridge.pairing.v1";

DATA_BLOB MakeBlob(const std::vector<std::uint8_t>& v) {
    DATA_BLOB b{};
    b.cbData = static_cast<DWORD>(v.size());
    b.pbData = const_cast<BYTE*>(reinterpret_cast<const BYTE*>(v.data()));
    return b;
}

DATA_BLOB EntropyBlob() {
    DATA_BLOB b{};
    b.cbData = static_cast<DWORD>(sizeof(kEntropy));
    b.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(kEntropy));
    return b;
}

}  // namespace

bool Protect(const std::vector<std::uint8_t>& plain, std::vector<std::uint8_t>& cipher) {
    if (plain.empty()) return false;
    DATA_BLOB in = MakeBlob(plain);
    DATA_BLOB ent = EntropyBlob();
    DATA_BLOB out{};
    if (!CryptProtectData(&in, L"PS5 Keyboard Bridge", &ent, nullptr, nullptr,
                          CRYPTPROTECT_UI_FORBIDDEN, &out))
        return false;
    cipher.assign(out.pbData, out.pbData + out.cbData);
    LocalFree(out.pbData);
    return true;
}

bool Unprotect(const std::vector<std::uint8_t>& cipher, std::vector<std::uint8_t>& plain) {
    if (cipher.empty()) return false;
    DATA_BLOB in = MakeBlob(cipher);
    DATA_BLOB ent = EntropyBlob();
    DATA_BLOB out{};
    if (!CryptUnprotectData(&in, nullptr, &ent, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &out))
        return false;
    plain.assign(out.pbData, out.pbData + out.cbData);
    SecureZeroMemory(out.pbData, out.cbData);
    LocalFree(out.pbData);
    return true;
}

bool SaveFile(const std::filesystem::path& path, const std::vector<std::uint8_t>& plain) {
    std::vector<std::uint8_t> cipher;
    if (!Protect(plain, cipher)) return false;

    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    if (ec) return false;

    const std::filesystem::path temp = path.string() + ".tmp";
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        out.write(reinterpret_cast<const char*>(cipher.data()), static_cast<std::streamsize>(cipher.size()));
        if (!out) return false;
    }
    std::filesystem::rename(temp, path, ec);
    if (ec) {
        std::filesystem::remove(temp, ec);
        return false;
    }
    return true;
}

std::optional<std::vector<std::uint8_t>> LoadFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return std::nullopt;
    std::vector<std::uint8_t> cipher((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    std::vector<std::uint8_t> plain;
    if (!Unprotect(cipher, plain)) return std::nullopt;
    return plain;
}

}  // namespace pskb::secure_store
