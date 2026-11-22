#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace pskb {

inline constexpr std::size_t kRegistKeySize = 16;
inline constexpr std::size_t kMorningSize = 16;

// What one successful PS5 pairing produces. It is the console's secret: store it only
// through SecureStore (DPAPI), never in plain settings and never in logs.
struct PairingInfo {
    std::uint32_t target = 0;                          // protocol target, PS5 = 1100
    std::array<char, kRegistKeySize> registKey{};      // hex text from the registration step
    std::array<std::uint8_t, kMorningSize> morning{};  // session key material
    std::string host;                                  // last known IPv4 address
    std::string name;                                  // console nickname
    std::string id;                                    // discovery host id, used to re-find a moved console

    // Wake-on-LAN credential: the registration key's hex digits as a number.
    std::uint64_t WakeCredential() const;

    bool IsComplete() const;

    std::vector<std::uint8_t> Serialize() const;
    static std::optional<PairingInfo> Deserialize(const std::vector<std::uint8_t>& blob);
};

}  // namespace pskb
