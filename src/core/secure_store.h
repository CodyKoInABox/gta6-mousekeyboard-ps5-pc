#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

namespace pskb::secure_store {

// Windows DPAPI, current-user scope. Data can only be decrypted by the same Windows user
// on the same PC, and the app-specific entropy stops other DPAPI users from reading it.
bool Protect(const std::vector<std::uint8_t>& plain, std::vector<std::uint8_t>& cipher);
bool Unprotect(const std::vector<std::uint8_t>& cipher, std::vector<std::uint8_t>& plain);

// Encrypts and writes atomically (temp file, then rename). Creates parent folders.
bool SaveFile(const std::filesystem::path& path, const std::vector<std::uint8_t>& plain);
std::optional<std::vector<std::uint8_t>> LoadFile(const std::filesystem::path& path);

}  // namespace pskb::secure_store
