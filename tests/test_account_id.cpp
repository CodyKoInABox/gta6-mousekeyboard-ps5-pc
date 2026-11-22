#include "core/account_id.h"
#include "test_framework.h"

using namespace pskb;

PSKB_TEST(Base64DecodesKnownVectors) {
    CHECK(Base64Decode("Zm9v").value() == "foo");
    CHECK(Base64Decode("Zm9vYg==").value() == "foob");
    CHECK(Base64Decode("Zm8=").value() == "fo");
    CHECK(Base64Decode("").value().empty());
}

PSKB_TEST(Base64RejectsMalformedInput) {
    CHECK(!Base64Decode("Zm9").has_value());
    CHECK(!Base64Decode("Zm9!").has_value());
    CHECK(!Base64Decode("Z=9v").has_value());
    CHECK(!Base64Decode("Zm==Zm9v").has_value());
}

PSKB_TEST(AccountIdFromDecimalIsLittleEndian) {
    const auto id = ParseAccountId("1");
    CHECK(id.has_value());
    CHECK_EQ(static_cast<int>((*id)[0]), 1);
    CHECK_EQ(static_cast<int>((*id)[1]), 0);
    CHECK_EQ(static_cast<int>((*id)[7]), 0);
}

PSKB_TEST(AccountIdFromDecimalUsesAllEightBytes) {
    const auto id = ParseAccountId("72057594037927936");  // 2^56
    CHECK(id.has_value());
    CHECK_EQ(static_cast<int>((*id)[7]), 1);
    CHECK_EQ(static_cast<int>((*id)[0]), 0);
}

PSKB_TEST(AccountIdFromBase64Form) {
    // "AQAAAAAAAAA=" decodes to the same eight bytes as decimal 1.
    const auto fromBase64 = ParseAccountId("AQAAAAAAAAA=");
    const auto fromDecimal = ParseAccountId("1");
    CHECK(fromBase64.has_value());
    CHECK(fromBase64 == fromDecimal);
}

PSKB_TEST(AccountIdIgnoresSurroundingWhitespace) {
    CHECK(ParseAccountId("  42  ").has_value());
    CHECK(ParseAccountId("\tAQAAAAAAAAA=\r\n").has_value());
}

PSKB_TEST(AccountIdRejectsInvalidInput) {
    CHECK(!ParseAccountId("").has_value());
    CHECK(!ParseAccountId("   ").has_value());
    CHECK(!ParseAccountId("0").has_value());
    CHECK(!ParseAccountId("abc").has_value());
    CHECK(!ParseAccountId("AAAAAAAAAAAA").has_value());     // 12 chars without padding: wrong size
    CHECK(!ParseAccountId("AAAAAAAAAAA=").has_value());     // all zero bytes
    CHECK(!ParseAccountId("99999999999999999999999").has_value());  // overflows 64 bits
}
