#include "core/account_id.h"

#include <cctype>
#include <cstring>
#include <limits>
#include <string>

namespace pskb {
namespace {

int Base64Value(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

std::string_view Trimmed(std::string_view s) {
    size_t begin = 0;
    size_t end = s.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(s[begin]))) ++begin;
    while (end > begin && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(begin, end - begin);
}

}  // namespace

std::optional<std::string> Base64Decode(std::string_view text) {
    if (text.size() % 4 != 0) return std::nullopt;
    std::string out;
    out.reserve(text.size() / 4 * 3);
    for (size_t i = 0; i < text.size(); i += 4) {
        int vals[4];
        int padding = 0;
        for (int j = 0; j < 4; ++j) {
            const char c = text[i + j];
            if (c == '=') {
                // '=' is only valid in the last one or two positions of the final quartet.
                if (i + 4 != text.size() || j < 2) return std::nullopt;
                vals[j] = 0;
                ++padding;
            } else {
                if (padding > 0) return std::nullopt;
                vals[j] = Base64Value(c);
                if (vals[j] < 0) return std::nullopt;
            }
        }
        const unsigned triple = (static_cast<unsigned>(vals[0]) << 18) |
                                (static_cast<unsigned>(vals[1]) << 12) |
                                (static_cast<unsigned>(vals[2]) << 6) | static_cast<unsigned>(vals[3]);
        out.push_back(static_cast<char>((triple >> 16) & 0xFF));
        if (padding < 2) out.push_back(static_cast<char>((triple >> 8) & 0xFF));
        if (padding < 1) out.push_back(static_cast<char>(triple & 0xFF));
    }
    return out;
}

std::optional<AccountId> ParseAccountId(std::string_view text) {
    const std::string_view s = Trimmed(text);
    if (s.empty()) return std::nullopt;

    bool allDigits = true;
    for (char c : s) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            allDigits = false;
            break;
        }
    }

    AccountId id{};
    if (allDigits) {
        std::uint64_t value = 0;
        for (char c : s) {
            const auto digit = static_cast<std::uint64_t>(c - '0');
            if (value > (std::numeric_limits<std::uint64_t>::max() - digit) / 10) return std::nullopt;
            value = value * 10 + digit;
        }
        if (value == 0) return std::nullopt;
        for (size_t i = 0; i < kAccountIdSize; ++i) {
            id[i] = static_cast<std::uint8_t>((value >> (8 * i)) & 0xFF);
        }
        return id;
    }

    const auto decoded = Base64Decode(s);
    if (!decoded || decoded->size() != kAccountIdSize) return std::nullopt;
    std::memcpy(id.data(), decoded->data(), kAccountIdSize);
    bool nonZero = false;
    for (auto byte : id) nonZero = nonZero || byte != 0;
    if (!nonZero) return std::nullopt;
    return id;
}

}  // namespace pskb
