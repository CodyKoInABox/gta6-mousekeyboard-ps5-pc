#include "core/psn_login.h"

#include <cctype>

namespace pskb::psn_login {
namespace {

constexpr std::string_view kScope =
    "psn:clientapp referenceDataService:countryConfig.read pushNotification:webSocket.desktop.connect "
    "sessionManager:remotePlaySession.system.update";

constexpr std::size_t kMaxCodeLength = 512;

bool IsCodeChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~';
}

std::string UrlEncodeSpaces(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (char c : text) {
        if (c == ' ') {
            out += "%20";
        } else {
            out += c;
        }
    }
    return out;
}

}  // namespace

std::string SignInUrl() {
    std::string url = "https://auth.api.sonyentertainmentnetwork.com/2.0/oauth/authorize?service_entity=urn:service-entity:psn"
                      "&response_type=code&client_id=";
    url += kClientId;
    url += "&redirect_uri=";
    url += kRedirectUri;
    url += "&scope=";
    url += UrlEncodeSpaces(kScope);
    url += "&request_locale=en_US&ui=pr&service_logo=ps&layout_type=popup&smcid=remoteplay&prompt=always"
           "&PlatformPrivacyWs1=minimal";
    return url;
}

std::optional<std::string> ExtractAuthCode(std::string_view pastedUrl) {
    const auto start = pastedUrl.find_first_not_of(" \t\r\n\"'");
    if (start == std::string_view::npos) return std::nullopt;
    std::string_view text = pastedUrl.substr(start);
    const auto end = text.find_last_not_of(" \t\r\n\"'");
    text = text.substr(0, end + 1);

    const auto question = text.find('?');
    if (question == std::string_view::npos) return std::nullopt;
    std::string_view query = text.substr(question + 1);
    if (const auto hash = query.find('#'); hash != std::string_view::npos) query = query.substr(0, hash);

    while (!query.empty()) {
        const auto amp = query.find('&');
        const std::string_view pair = query.substr(0, amp);
        const auto eq = pair.find('=');
        if (eq != std::string_view::npos && pair.substr(0, eq) == "code") {
            const std::string_view value = pair.substr(eq + 1);
            if (value.empty() || value.size() > kMaxCodeLength) return std::nullopt;
            for (char c : value) {
                if (!IsCodeChar(c)) return std::nullopt;
            }
            return std::string(value);
        }
        if (amp == std::string_view::npos) break;
        query = query.substr(amp + 1);
    }
    return std::nullopt;
}

std::string TokenRequestBody(std::string_view code) {
    std::string body = "grant_type=authorization_code&code=";
    body += code;
    body += "&scope=";
    body += UrlEncodeSpaces(kScope);
    body += "&redirect_uri=";
    body += kRedirectUri;
    return body;
}

std::optional<std::string> JsonScalarField(std::string_view json, std::string_view key) {
    const std::string needle = "\"" + std::string(key) + "\"";
    const auto keyPos = json.find(needle);
    if (keyPos == std::string_view::npos) return std::nullopt;

    std::size_t i = keyPos + needle.size();
    while (i < json.size() && std::isspace(static_cast<unsigned char>(json[i]))) ++i;
    if (i >= json.size() || json[i] != ':') return std::nullopt;
    ++i;
    while (i < json.size() && std::isspace(static_cast<unsigned char>(json[i]))) ++i;
    if (i >= json.size()) return std::nullopt;

    if (json[i] == '"') {
        const auto close = json.find('"', i + 1);
        if (close == std::string_view::npos) return std::nullopt;
        const std::string_view value = json.substr(i + 1, close - i - 1);
        if (value.empty() || value.find('\\') != std::string_view::npos) return std::nullopt;
        return std::string(value);
    }

    std::size_t end = i;
    while (end < json.size() && std::isdigit(static_cast<unsigned char>(json[end]))) ++end;
    if (end == i) return std::nullopt;
    return std::string(json.substr(i, end - i));
}

std::string Base64Encode(std::string_view bytes) {
    static constexpr char kAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve((bytes.size() + 2) / 3 * 4);
    for (std::size_t i = 0; i < bytes.size(); i += 3) {
        const auto b0 = static_cast<std::uint8_t>(bytes[i]);
        const std::uint32_t b1 = i + 1 < bytes.size() ? static_cast<std::uint8_t>(bytes[i + 1]) : 0;
        const std::uint32_t b2 = i + 2 < bytes.size() ? static_cast<std::uint8_t>(bytes[i + 2]) : 0;
        const std::uint32_t triple = (static_cast<std::uint32_t>(b0) << 16) | (b1 << 8) | b2;
        out += kAlphabet[(triple >> 18) & 0x3F];
        out += kAlphabet[(triple >> 12) & 0x3F];
        out += i + 1 < bytes.size() ? kAlphabet[(triple >> 6) & 0x3F] : '=';
        out += i + 2 < bytes.size() ? kAlphabet[triple & 0x3F] : '=';
    }
    return out;
}

std::string AccountIdToBase64(const AccountId& id) {
    return Base64Encode(std::string_view(reinterpret_cast<const char*>(id.data()), id.size()));
}

}  // namespace pskb::psn_login
