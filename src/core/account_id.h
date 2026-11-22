#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace pskb {

inline constexpr std::size_t kAccountIdSize = 8;
using AccountId = std::array<std::uint8_t, kAccountIdSize>;

// Accepts the two forms the PS5 Remote Play flow uses:
//   * decimal: all digits, read as an unsigned 64-bit number (little-endian bytes)
//   * base64:  the 12-character form ending in '=', decoding to 8 bytes
// Surrounding whitespace is ignored. Returns nothing for any other input or a zero ID.
std::optional<AccountId> ParseAccountId(std::string_view text);

// Standard base64 decoding (RFC 4648, padded). Returns nothing on malformed input.
std::optional<std::string> Base64Decode(std::string_view text);

}  // namespace pskb
