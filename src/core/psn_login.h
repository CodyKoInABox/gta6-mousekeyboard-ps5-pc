#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "core/account_id.h"

namespace pskb::psn_login {

// Public client identifiers of the official Remote Play client, the same values chiaki-ng ships.
// They identify the app to Sony; they grant nothing by themselves, and the user's sign-in
// happens on Sony's own page in the browser.
inline constexpr std::string_view kClientId = "ba495a24-818c-472b-b12d-ff231c1b5745";
inline constexpr std::string_view kClientSecret = "mvaiZkRsAsI1IBkY";
inline constexpr std::string_view kRedirectUri = "https://remoteplay.dl.playstation.net/remoteplay/redirect";
inline constexpr std::string_view kTokenUrl = "https://auth.api.sonyentertainmentnetwork.com/2.0/oauth/token";

// Sony's sign-in page. The user types their password there, never into this app.
std::string SignInUrl();

// Pulls the one-time code from the redirect URL the user pastes back. Returns nothing when the
// text is not a URL with a well-formed code parameter.
std::optional<std::string> ExtractAuthCode(std::string_view pastedUrl);

// Form body for exchanging the code for a token.
std::string TokenRequestBody(std::string_view code);

// Reads a top-level field from one of Sony's small JSON replies. Accepts a quoted string or a
// bare number and returns its text. Not a general JSON parser: it only supports the flat
// objects these endpoints return and refuses escaped strings.
std::optional<std::string> JsonScalarField(std::string_view json, std::string_view key);

// Standard base64 (RFC 4648, padded).
std::string Base64Encode(std::string_view bytes);

// The 8-byte account ID in the base64 form chiaki-ng and the PS5 tools show.
std::string AccountIdToBase64(const AccountId& id);

}  // namespace pskb::psn_login
