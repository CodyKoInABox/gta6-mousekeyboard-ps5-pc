#pragma once

#include <string>
#include <string_view>

#include "core/account_id.h"

namespace pskb::psn_signin {

struct SignInResult {
    bool ok = false;
    AccountId account{};
    std::string message;  // plain language, safe to show; never contains tokens or the code
};

// Opens Sony's sign-in page in the default browser. The password is typed only there.
bool OpenSignInPage();

// Exchanges the redirect URL the user pasted back for their PSN account ID. Talks to Sony over
// HTTPS only. The one-time code and the access token are wiped from memory before returning.
SignInResult CompleteSignIn(std::string_view pastedUrl);

}  // namespace pskb::psn_signin
