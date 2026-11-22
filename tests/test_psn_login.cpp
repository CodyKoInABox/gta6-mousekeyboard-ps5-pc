#include "core/psn_login.h"
#include "test_framework.h"

using namespace pskb;

PSKB_TEST(SignInUrlUsesSonyLoginAndRemotePlayClient) {
    const std::string url = psn_login::SignInUrl();
    CHECK(url.rfind("https://auth.api.sonyentertainmentnetwork.com/2.0/oauth/authorize?", 0) == 0);
    CHECK(url.find("response_type=code") != std::string::npos);
    CHECK(url.find("client_id=ba495a24-818c-472b-b12d-ff231c1b5745") != std::string::npos);
    CHECK(url.find("scope=psn:clientapp%20referenceDataService") != std::string::npos);
    CHECK(url.find(' ') == std::string::npos);
}

PSKB_TEST(ExtractsCodeFromPastedRedirectUrl) {
    const auto code = psn_login::ExtractAuthCode(
        "https://remoteplay.dl.playstation.net/remoteplay/redirect?code=v3.AbC-123_x.9~z&cid=abc");
    CHECK(code.has_value());
    CHECK(*code == "v3.AbC-123_x.9~z");
}

PSKB_TEST(ExtractsCodeWhenItIsNotFirstAndTextIsWrapped) {
    const auto code = psn_login::ExtractAuthCode(
        "  \"https://remoteplay.dl.playstation.net/remoteplay/redirect?state=1&code=ABC123#frag\"\r\n");
    CHECK(code.has_value());
    CHECK(*code == "ABC123");
}

PSKB_TEST(RejectsUrlsWithoutAUsableCode) {
    CHECK(!psn_login::ExtractAuthCode("").has_value());
    CHECK(!psn_login::ExtractAuthCode("not a url").has_value());
    CHECK(!psn_login::ExtractAuthCode("https://example.com/redirect?state=1").has_value());
    CHECK(!psn_login::ExtractAuthCode("https://example.com/redirect?code=").has_value());
    CHECK(!psn_login::ExtractAuthCode("https://example.com/redirect?code=bad%20value").has_value());
    CHECK(!psn_login::ExtractAuthCode("https://example.com/redirect?code=a<script>").has_value());
    CHECK(!psn_login::ExtractAuthCode("https://example.com/?code=" + std::string(600, 'a')).has_value());
}

PSKB_TEST(TokenBodyCarriesCodeAndRedirect) {
    const std::string body = psn_login::TokenRequestBody("CODE1");
    CHECK(body.rfind("grant_type=authorization_code&code=CODE1&", 0) == 0);
    CHECK(body.find("redirect_uri=https://remoteplay.dl.playstation.net/remoteplay/redirect") != std::string::npos);
    CHECK(body.find("scope=psn:clientapp%20") != std::string::npos);
}

PSKB_TEST(JsonFieldReadsStringsAndNumbers) {
    const std::string tokenReply = R"({"access_token":"AT.abc-DEF_123","expires_in":3599,"token_type":"bearer"})";
    CHECK(psn_login::JsonScalarField(tokenReply, "access_token").value() == "AT.abc-DEF_123");
    CHECK(psn_login::JsonScalarField(tokenReply, "expires_in").value() == "3599");

    const std::string accountReply = R"({ "user_id" : "72057594037927936", "online_id":"x"})";
    CHECK(psn_login::JsonScalarField(accountReply, "user_id").value() == "72057594037927936");

    const std::string numericReply = R"({"user_id":42})";
    CHECK(psn_login::JsonScalarField(numericReply, "user_id").value() == "42");
}

PSKB_TEST(JsonFieldRefusesMissingOrEscapedValues) {
    CHECK(!psn_login::JsonScalarField("{\"error\":\"invalid_grant\"}", "access_token").has_value());
    CHECK(!psn_login::JsonScalarField("{\"access_token\":\"\"}", "access_token").has_value());
    CHECK(!psn_login::JsonScalarField("{\"access_token\":\"a\\\"b\"}", "access_token").has_value());
    CHECK(!psn_login::JsonScalarField("{\"user_id\":", "user_id").has_value());
    CHECK(!psn_login::JsonScalarField("", "user_id").has_value());
}

PSKB_TEST(AccountIdBase64MatchesKnownVectors) {
    AccountId one{};
    one[0] = 1;
    CHECK(psn_login::AccountIdToBase64(one) == "AQAAAAAAAAA=");

    AccountId allFf{};
    allFf.fill(0xFF);
    CHECK(psn_login::AccountIdToBase64(allFf) == "//////////8=");
}

PSKB_TEST(Base64EncodeMatchesRfcVectors) {
    CHECK(psn_login::Base64Encode("") == "");
    CHECK(psn_login::Base64Encode("f") == "Zg==");
    CHECK(psn_login::Base64Encode("fo") == "Zm8=");
    CHECK(psn_login::Base64Encode("foo") == "Zm9v");
    CHECK(psn_login::Base64Encode("foob") == "Zm9vYg==");
    CHECK(psn_login::Base64Encode("foobar") == "Zm9vYmFy");
}

PSKB_TEST(AccountIdBase64RoundTripsThroughParser) {
    const auto id = ParseAccountId("72057594037927936");
    CHECK(id.has_value());
    const std::string text = psn_login::AccountIdToBase64(*id);
    const auto back = ParseAccountId(text);
    CHECK(back.has_value());
    CHECK(*back == *id);
}
